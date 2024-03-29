#include "MIDIInput.h"

#include "Event.h"
#include "Logger.h"
#include "MIDIPlayer.h"

float MIDIInput::ticks_per_second(MIDIPlayer const& player) const
{
    return static_cast<float>(ticks_per_quarter_note()) / player.microseconds_per_quarter_note() * 1000000;
}

std::unique_ptr<Event> MIDIInput::read_channeled_event(std::istream& in, uint8_t type, uint8_t channel)
{
    switch (type) {
        case 0x80: // Note Off
        case 0x90: // Note On
        {
            uint8_t key = 0;
            if (!in.read((char*)&key, 1))
                return {};
            uint8_t velocity = 0;
            if (!in.read((char*)&velocity, 1))
                return {};
            return std::make_unique<NoteEvent>(type == 0x80 || velocity == 0 ? NoteEvent::Type::Off : NoteEvent::Type::On, channel, key & 0x7f, velocity & 0x7f);
        }
        case 0xa0: // Polyphonic Key Pressure (Aftertouch)
        {
            uint8_t key = 0;
            if (!in.read((char*)&key, 1))
                return {};
            uint8_t pressure = 0;
            if (!in.read((char*)&pressure, 1))
                return {};
            break; // TODO
        }
        case 0xb0: // Control Change
        {
            uint8_t number = 0;
            if (!in.read((char*)&number, 1))
                return {};
            uint8_t value = 0;
            if (!in.read((char*)&value, 1))
                return {};
            if (number >= (uint8_t)ControlChangeEvent::Number::Count) {
                logger::error("Invalid Control Change Number");
                return std::make_unique<InvalidEvent>(type);
            }
            if (number < 0x40) {
                return std::make_unique<LMControlChangeEvent>(channel, number & 0x20 ? LMControlChangeEvent::ByteIndex::MSB : LMControlChangeEvent::ByteIndex::LSB,
                    static_cast<LMControlChangeEvent::Number>(number), value & 0x7f);
            }
            return std::make_unique<ControlChangeEvent>(channel, static_cast<ControlChangeEvent::Number>(number), value & 0x7f);
        }
        case 0xc0: // Program Change
        {
            uint8_t program = 0;
            if (!in.read((char*)&program, 1))
                return {};
            return std::make_unique<ProgramChangeEvent>(channel, program);
        }
        case 0xd0: // Channel Pressure (Aftertouch)
            // TODO
            if (!in.ignore(1))
                return {};
            break;
        case 0xe0: // Pitch Wheel Change
            // TODO
            if (!in.ignore(2))
                return {};
            break;
    }
    return std::make_unique<InvalidEvent>(type);
}

std::unique_ptr<Event> MIDIInput::read_event(std::istream& in)
{
    uint8_t status = 0;
    if (!(in.read((char*)&status, 1))) {
        logger::error("failed to read status number");
        return {};
    }

    // Appendix 1.1 - Table of Major MIDI Messages
    if (status >= 0x80 && status <= 0xef)
        return read_channeled_event(in, status & 0xf0, status & 0x0f);

    if (status != 0xfe)
        logger::error("Invalid status number: {:#x}", (int)status);
    return std::make_unique<InvalidEvent>(status);
}

std::vector<Event*> MIDIInput::find_events_in_range(size_t start_tick, size_t end_tick) const
{
    std::vector<Event*> out;
    for (auto& track : m_tracks) {
        auto track_out = track.find_events_in_range(start_tick, end_tick);
        out.insert(out.begin(), track_out.begin(), track_out.end());
    }

    // Ensure that events will be played in the correct time order
    std::stable_sort(out.begin(), out.end(), [](Event* l, Event* r) {
        return l->tick() < r->tick();
    });

    return out;
}
