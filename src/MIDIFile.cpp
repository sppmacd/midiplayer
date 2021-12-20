#include "MIDIFile.h"

#include "Event.h"
#include "MIDIPlayer.h"
#include <bit>

using namespace std::literals;

void MIDIFileInput::dump() const
{
    std::cerr << "MIDI file format=" << static_cast<int>(m_format) << ", smpte=" << m_is_smpte << std::endl;
    if(m_is_smpte)
        std::cerr << "negative_smpte_format=" << m_negative_smpte_format << ", ticks_per_frame=" << m_ticks_per_frame << std::endl;

    else
        std::cerr << "ticks_per_quarter_note=" << m_ticks_per_quarter_note << std::endl;
    std::cerr << "track count: " << m_tracks.size() << std::endl;
}

bool MIDIFileInput::read_midi(std::istream& in)
{
    if(!in.good())
    {
        std::cerr << "ERROR: Empty or invalid stream" << std::endl;
        return false;
    }

    while(!in.eof())
    {
        if(!read_chunk(in))
            return false;
    }

    return true;
}

size_t MIDIFileInput::ticks_per_frame(MIDIPlayer& player) const
{
    // TODO: Don't assume 60 FPS
    return (static_cast<double>(ticks_per_quarter_note()) / player.microseconds_per_quarter_note()) / (60 / 1000000.0);
}

#define ERROR(msg) do { std::cerr << "ERROR: Failed to " << msg << std::endl; return {}; } while(false)

// 1.3 - Chunks
bool MIDIFileInput::read_chunk(std::istream& in)
{
    char type[4]; uint32_t length;
    if(!in.read(type, 4))
        return in.eof();
    if(!in.read((char*)&length, 4))
        ERROR("read chunk length");
    
    length = htobe32(length);

    std::string_view type_sv = type;
    if(type_sv == "MThd"sv)
    {
        if(m_header_encountered)
        {
            std::cerr << "ERROR: Header encountered twice" << std::endl;
            return false;
        }
        m_header_encountered = true;
        return read_header(in);
    }
    if(type_sv == "MTrk"sv)
    {
        if(!m_header_encountered)
        {
            std::cerr << "ERROR: MTrk without header" << std::endl;
            return false;
        }
        return read_track_data(in, length);
    }

    // Your programs should EXPECT alien chunks and treat them as if they weren't there
    std::cerr << "Ignoring alien chunk (" << length << " bytes)" << std::endl;
    if(!in.ignore(length))
        ERROR("ignore alien chunk");

    return true;
}

// 2.1 - Header Chunks
bool MIDIFileInput::read_header(std::istream& in)
{
    uint16_t format, ntrks, division;
    if(!in.read((char*)&format, 2))
        ERROR("read format");
    if(!in.read((char*)&ntrks, 2))
        ERROR("read ntrks");
    if(!in.read((char*)&division, 2))
        ERROR("read division");

    format = htobe16(format);
    ntrks = htobe16(ntrks);
    division = htobe16(division);

    if(format > 2)
    {
        std::cerr << "ERROR: Invalid format value" << std::endl;
        return false;
    }
    m_format = static_cast<MIDIFileFormat>(format);
    if(m_format == MIDIFileFormat::SingleMultichannelTrack && ntrks > 1)
    {
        std::cerr << "ERROR: Multiple tracks in single multichannel track format" << std::endl;
        return false;
    }
    m_tracks.reserve(ntrks);

    if(division & 0x8000)
    {
        m_is_smpte = true;
        m_negative_smpte_format = division & 0x7f00 >> 8;
        m_ticks_per_frame = division & 0xff;
    }
    else
    {
        m_is_smpte = false;
        m_ticks_per_quarter_note = division & 0x7fff;
    }

    return true;
}

// 2.3 - Track Chunks
bool MIDIFileInput::read_track_data(std::istream& in, size_t length)
{
    // at least one MTrk event must be present
    if(length == 0)
    {
        std::cerr << "ERROR: No events in track" << std::endl;
        return false;
    }
    Track track;
    size_t offset = in.tellg();
    size_t end = offset + length;
    size_t current_tick = 0;
    while(offset < end)
    {
        auto delta_time = read_variable_length_quantity(in);
        if(!delta_time.has_value())
            ERROR("read delta time");
        current_tick += delta_time.value();

        auto event = [&]()->std::unique_ptr<Event> {
            uint8_t status = 0;
            if(!(in.read((char*)&status, 1)))
                ERROR("read status");
            // 3 - Meta-Events
            if(status == 0xff)
            {
                uint8_t type = 0;
                if(!(in.read((char*)&type, 1)))
                    return {};
                return read_meta_event(in, type);
            }

            // Appendix 1.1 - Table of Major MIDI Messages
            if(status >= 0x80 && status <= 0xef)
                return read_channeled_event(in, status & 0xf0, status & 0x0f);

            std::cerr << "ERROR: Invalid status number: " << std::hex << (int)status << std::dec << std::endl;
            return std::make_unique<InvalidEvent>(status);
        }();
        if(!event)
            ERROR("read event");
        event->set_tick(current_tick);
        event->dump();
        track.add_event(std::move(event));
        offset = in.tellg();
    }
    m_tracks.push_back(std::move(track));
    return true;
}

// 1.1 - Variable Length Quantity
std::optional<uint32_t> MIDIFileInput::read_variable_length_quantity(std::istream& in) const
{
    uint32_t value = 0;
    while(true)
    {
        uint8_t byte = 0;
        if(!(in.read((char*)&byte, 1)))
            ERROR("read vlq");

        value <<= 7;
        value |= byte & 0x7f;
        if(!(byte & 0x80))
            return value;
    }
    return {};
}

std::unique_ptr<Event> MIDIFileInput::read_meta_event(std::istream& in, uint8_t type)
{
    auto read_string = [](std::istream& in, uint8_t len)->std::optional<std::string> {
        std::string data;
        data.resize(len);
        if(!in.read(data.data(), len))
            return {};
        return data;
    };

    auto len = read_variable_length_quantity(in);
    if(!len.has_value())
        return {};

    std::cerr << "meta-event type=" << std::hex << (int)type << std::dec << " len=" << len.value() << std::endl;
    // 3.1 - Meta-Event Definitions
    switch(type)
    {
        case 0x00: // Sequence Number
            // "This optional event, which must occur at the beginning of a track"
            std::cerr << "Sequence Number" << std::endl;
            in.ignore(1);
            return {};
        case 0x01: // Text Event
        case 0x02: // Copyright Notice
        case 0x03: // Sequence/Track Name
        case 0x04: // Instrument Name
        case 0x05: // Lyric
        case 0x06: // Marker
        case 0x07: // Cue Point
        {
            auto data = read_string(in, len.value());
            if(!data.has_value())
                return {};
            return std::make_unique<TextEvent>(static_cast<TextEvent::Type>(type - 1), data.value());
        }
        case 0x20: // MIDI Channel Prefix
            break;
        case 0x2f: // End of Track
            return std::make_unique<EndOfTrackEvent>();
        case 0x51: // Set Tempo
        {
            uint32_t tempo_val = 0;
            for(int i = 0; i < 3; i++)
            {
                uint8_t data = 0;
                if(!in.read((char*)&data, 1))
                    return {};
                tempo_val <<= 8;
                tempo_val |= data;
            }
            return std::make_unique<SetTempoEvent>(tempo_val);
        }
        case 0x54: // SMPTE Offset
            break;
        case 0x58: // Time Signature
        {
            uint8_t numerator, denominator, clocks_per_metronome_click, _32s_in_quarter_note;
            if(!(in >> numerator >> denominator >> clocks_per_metronome_click >> _32s_in_quarter_note))
                return {};
            // FIXME: The denominator is a negative power of two: 2 represents a quarter-note, 3 represents an eighth-note
            return std::make_unique<TimeSignatureEvent>(numerator, 1 << denominator, clocks_per_metronome_click, _32s_in_quarter_note);
        }
        case 0x59: // Key Signature
            break;
        case 0x7f: // Sequencer Specific Meta-Event
            break;
    }
    if(!in.ignore(len.value()))
        return {};
    return std::make_unique<InvalidEvent>(type);
}

////////////
// OUTPUT //
////////////

MIDIFileOutput::MIDIFileOutput(std::string path)
: m_output{path}
{
    if(m_output.fail())
    {
        std::cerr << "ERROR: Failed to open MIDI output file" << std::endl;
        return;
    }
    std::cerr << "INFO: Enabled MIDI output to " << path << std::endl;

    // Header
    m_output.write("MThd", 4); // Signature
    m_output.write("\0\0\0\6", 4); // Length
    m_output.write("\0\0", 2); // Format
    m_output.write("\0\1", 2); // Track count
    uint16_t ticks_per_quarter_note = htobe16(192);
    m_output.write((char*)&ticks_per_quarter_note, 2);

    // Track header
    m_output.write("MTrk", 4);
    m_track_length_offset = m_output.tellp();
    m_output.write("\0\0\0\0", 4); // Length (To be filled out later)
    m_output.flush();
}

MIDIFileOutput::~MIDIFileOutput()
{
    // Write End Of Track event
    EndOfTrackEvent event;
    event.set_tick(m_last_tick + 192);
    write_event(event);
    std::cerr << "MIDIFileOutput: Closed successfully" << std::endl;
}

void MIDIFileOutput::write_event(Event const& event)
{
    size_t offset = m_output.tellp();
    write_variable_length_quantity(event.tick() - m_last_tick);
    event.serialize(m_output);

    // write length
    size_t event_size = (size_t)m_output.tellp() - offset;
    m_track_length += event_size;

    uint32_t track_length_be = htobe32(m_track_length);

    m_output.seekp(m_track_length_offset);
    m_output.write((char*)&track_length_be, 4);
    m_output.seekp(offset + event_size);

    m_last_tick = event.tick();
}

void MIDIFileOutput::write_variable_length_quantity(uint32_t value)
{
    uint32_t reversed_value = 0;
    while(value > 0)
    {
        reversed_value |= value & 0x7f;
        value >>= 7;
        reversed_value <<= 7;
    }
    while(reversed_value > 127)
    {
        m_output.put(0x80 | (reversed_value & 0x7f));
        reversed_value >>= 7;
    }
    m_output.put(reversed_value & 0x7f);
}
