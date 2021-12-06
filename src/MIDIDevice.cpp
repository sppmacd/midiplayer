#include "MIDIDevice.h"

#include "MIDIPlayer.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include <fcntl.h>

MIDIDevice::MIDIDevice(std::string const& path)
{
    m_tracks.resize(1);
    m_io_thread = std::jthread([this, path]() {
        std::ifstream m_file(path);
        if(m_file.fail())
        {
            std::cerr << "Failed to open MIDI device " << path << ": " << strerror(errno) << std::endl;
            return;
        }
        m_valid.store(true, std::memory_order_relaxed);
        while(true)
        {
            auto event = MIDI::read_event(m_file);
            if(!event)
            {
                std::cerr << "Failed to read event" << std::endl;
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
    assert(m_player);
    while(auto event = read_event())
    {
        // Do not store invalid events
        if(dynamic_cast<InvalidEvent*>(event.get()))
            continue;
        event->set_tick(m_player->current_tick());
        m_tracks[0].add_event(std::move(event));
    }
}
