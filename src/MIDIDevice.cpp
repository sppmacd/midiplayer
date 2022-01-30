#include "MIDIDevice.h"

#include "Logger.h"
#include "MIDIPlayer.h"
#include "SpanStream.h"

#include <atomic>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <sstream>

using namespace std::chrono_literals;

MIDIDeviceInput::MIDIDeviceInput(int port)
{
    m_tracks.resize(1);

    m_input.setClientName("MIDIPlayer");
    m_input.setCallback([](double, std::vector<uint8_t>* data, void* user_data)
        {
            assert(user_data);
            assert(data);
            MIDIDeviceInput* this_ = reinterpret_cast<MIDIDeviceInput*>(user_data);
            auto& player = *this_->m_player.load();
    
            ISpanStream stream({data->data(), data->size()});
            auto event = this_->MIDIInput::read_event(stream);
            if(!event)
            {
                logger::error("Failed to read event");
                this_->m_valid.store(false, std::memory_order_relaxed);
                return;
            }
            event->set_tick(this_->current_tick(player));
    
            std::lock_guard lock{this_->m_event_queue_mutex};
            this_->m_event_queue.push(std::move(event)); },
        this);
    try
    {
        m_input.openPort(port);
    }
    catch(...)
    {
        return;
    }
    // FIXME: RtMidi what if it is actually NOT valid?
    m_valid = true;
}

std::unique_ptr<Event> MIDIDeviceInput::read_event()
{
    std::lock_guard lock { m_event_queue_mutex };
    if(m_event_queue.empty())
        return nullptr;
    auto event = std::move(m_event_queue.front());
    m_event_queue.pop();
    return event;
}

void MIDIDeviceInput::update(MIDIPlayer& player)
{
    m_player = &player;
    while(auto event = read_event())
    {
        // Do not store invalid events
        if(dynamic_cast<InvalidEvent*>(event.get()))
            continue;
        m_tracks[0].add_event(std::move(event));
    }
}

size_t MIDIDeviceInput::current_tick(MIDIPlayer const& player) const
{
    auto time = (std::chrono::system_clock::now() - player.start_time());
    return time / 1us * ((double)ticks_per_quarter_note() / player.microseconds_per_quarter_note());
}

////////////
// OUTPUT //
////////////

MIDIDeviceOutput::MIDIDeviceOutput(int out)
{
    m_output.setClientName("MIDIPlayer");
    m_output.openPort(out);
    m_valid = true;
}

void MIDIDeviceOutput::write_event(Event const& event)
{
    std::ostringstream oss;
    event.serialize(oss);
    if(oss.str().size() == 0)
        return;

    // FIXME: This copy is unnecessary, but RtMidi expects data in form of std::vector.
    //        This whole API is weird, but probably would need to patch RtMidi to fix.
    std::vector<uint8_t> data(oss.str().size());
    memcpy(data.data(), oss.str().data(), oss.str().size());
    m_output.sendMessage(&data);
}

namespace MIDI
{

void print_available_ports()
{
    std::cerr << "Input: " << std::endl;
    RtMidiIn in;
    for(int i = 0; i < in.getPortCount(); i++)
        std::cerr << i << ": " << in.getPortName(i) << std::endl;

    std::cerr << "Output: " << std::endl;
    RtMidiOut out;
    for(int i = 0; i < in.getPortCount(); i++)
        std::cerr << i << ": " << out.getPortName(i) << std::endl;
}

}
