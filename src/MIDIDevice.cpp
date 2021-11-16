#include "MIDIDevice.h"

#include <atomic>
#include <cstring>
#include <ext/stdio_filebuf.h>
#include <fcntl.h>

MIDIDevice::MIDIDevice(std::string const& path)
{
    m_tracks.resize(1);
    m_io_thread = std::jthread([this, path]() {
        std::ifstream m_file(path);
        if(m_file.fail())
        {
            std::cout << "Failed to open MIDI device " << path << ": " << strerror(errno) << std::endl;
            return;
        }
        m_valid.store(true, std::memory_order_relaxed);
        while(true)
        {
            auto event = MIDI::read_event(m_file);
            if(!event)
            {
                std::cout << "Failed to read event" << std::endl;
                m_valid.store(false, std::memory_order_relaxed);
                return;
            }
            std::lock_guard lock{m_event_queue_mutex};
            m_event_queue.push(std::move(event));
        }
    });
}

std::unique_ptr<Event> MIDIDevice::read_event()
{
    std::lock_guard lock{m_event_queue_mutex};
    if(m_event_queue.empty())
        return nullptr;
    auto event = std::move(m_event_queue.front());
    m_event_queue.pop();
    return event;
}

void MIDIDevice::update()
{
    while(auto event = read_event())
    {
        event->set_tick(m_tick);
        m_tracks[0].add_event(std::move(event));
    }
    m_tick++;
}
