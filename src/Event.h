#pragma once

#include "Config/Property.h"
#include "MIDIKey.h"

#include <SFML/Graphics.hpp>
#include <bit>
#include <iostream>
#include <memory>
#include <optional>

class MIDIPlayer;

class Event
{
public:
    virtual ~Event() = default;

    void set_tick(size_t tick) { m_tick = tick; }
    size_t tick() const { return m_tick; }

    virtual void dump() const = 0;
    virtual void render(MIDIPlayer&, sf::RenderTarget&) {}
    virtual void execute(MIDIPlayer&) = 0;

    virtual bool is_serializable() const { return false; }
    virtual void serialize(std::ostream&) const {}

    virtual std::unique_ptr<Event> clone() const = 0;
    virtual Config::NamedFormalParameters formal_parameters() const { return {}; }
    virtual bool read_from_parameters(Config::NamedParameters const&) { return false; }

    static std::unique_ptr<Event> create_event_from_name(std::string_view);

private:
    size_t m_tick { 0 };
};

class EndOfTrackEvent : public Event
{
public:
    virtual void dump() const override { std::cerr << "End Of Track Event" << std::endl; }
    virtual void render(MIDIPlayer& player, sf::RenderTarget&) override;
    virtual void execute(MIDIPlayer&) override;

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        std::cerr << "Writing EndOfTrack" << std::endl;
        stream.put(-1);   // Meta-event
        stream.put(0x2f); // End Of Track
        stream.put(0);    // size: vlq 1
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<EndOfTrackEvent>(*this); }
};

class InvalidEvent : public Event
{
public:
    InvalidEvent(uint8_t type)
    : m_type(type) {}

    virtual void dump() const override { std::cerr << "Invalid Event " << std::hex << (int)m_type << std::dec << std::endl; }
    virtual void execute(MIDIPlayer&) override {}

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<InvalidEvent>(*this); }

private:
    uint8_t m_type;
};

class TextEvent : public Event
{
public:
    enum class Type
    {
        Text,
        Copyright,
        TrackName,
        Instrument,
        Lyric,
        Marker,
        CuePoint
    };

    TextEvent(Type type = {}, std::string const& text = {})
    : m_type(type), m_text(text) {}

    virtual void dump() const override { std::cerr << "Text Event " << static_cast<int>(m_type) << ": " << m_text << std::endl; }
    virtual void execute(MIDIPlayer&) override;

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<TextEvent>(*this); }
    virtual Config::NamedFormalParameters formal_parameters() const override;
    virtual bool read_from_parameters(Config::NamedParameters const&) override;

private:
    Type m_type;
    std::string m_text;
};

class SetTempoEvent : public Event
{
public:
    SetTempoEvent(uint32_t microseconds_per_quarter_note)
    : m_microseconds_per_quarter_note(microseconds_per_quarter_note) {}

    virtual void dump() const override { std::cerr << "Set Tempo Event " << m_microseconds_per_quarter_note << std::endl; }
    virtual void execute(MIDIPlayer&) override;

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        std::cerr << "Writing SetTempo" << std::endl;
        stream.put(-1);   // Meta-event
        stream.put(0x51); // Set Tempo
        stream.put(3);    // size: vlq 3
        stream.put((m_microseconds_per_quarter_note >> 16) & 0xff);
        stream.put((m_microseconds_per_quarter_note >> 8) & 0xff);
        stream.put(m_microseconds_per_quarter_note & 0xff);
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<SetTempoEvent>(*this); }

private:
    uint32_t m_microseconds_per_quarter_note;
};

class TimeSignatureEvent : public Event
{
public:
    TimeSignatureEvent(unsigned numerator, unsigned denominator, uint8_t clocks_per_metronome_click, uint8_t _32s_in_quarter_note)
    : m_numerator(numerator), m_denominator(denominator), m_clocks_per_metronome_click(clocks_per_metronome_click), m_32s_in_quarter_note(_32s_in_quarter_note) {}

    virtual void dump() const override
    {
        std::cerr << "Time Signature Event " << static_cast<int>(m_numerator) << "/" << static_cast<int>(m_denominator) << std::endl;
    }
    virtual void execute(MIDIPlayer&) override
    { /* TODO */
    }

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        std::cerr << "Writing TimeSignature" << std::endl;
        stream.put(-1);   // Meta-event
        stream.put(0x58); // Time Signature
        stream.put(4);    // size: vlq 4
        stream.put(m_numerator);
        stream.put(std::countr_zero(m_denominator));
        stream.put(m_clocks_per_metronome_click);
        stream.put(m_32s_in_quarter_note);
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<TimeSignatureEvent>(*this); }

private:
    unsigned m_numerator;
    unsigned m_denominator;
    uint8_t m_clocks_per_metronome_click;
    uint8_t m_32s_in_quarter_note;
};

using MIDIChannel = uint8_t;

class NoteEvent : public Event
{
public:
    enum class Type
    {
        On,
        Off
    };

    struct TransitionUnit
    {
        MIDIKey key;
        MIDIChannel channel;
    };

    NoteEvent(Type type, MIDIChannel channel, MIDIKey key, uint8_t velocity)
    : m_type(type), m_channel(channel), m_key(key), m_velocity(velocity) {}

    virtual void dump() const override
    {
        std::cerr << tick() << ": Note " << (m_type == Type::On ? "On" : "Off") << " Event channel=" << (int)m_channel
                  << ", key=" << (int)m_key << ", velocity=" << (int)m_velocity << std::endl;
    }

    virtual void render(MIDIPlayer& player, sf::RenderTarget&) override;
    virtual void execute(MIDIPlayer&) override;

    MIDIChannel channel() const { return m_channel; }
    MIDIKey key() const { return m_key; }
    uint8_t velocity() const { return m_velocity; }
    TransitionUnit transition_unit() const { return { m_key, m_channel }; }

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        if(m_type == Type::On)
            stream << (uint8_t)(0x90 + m_channel);
        else
            stream << (uint8_t)(0x80 + m_channel);

        stream << m_key << m_velocity;
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<NoteEvent>(*this); }

private:
    Type m_type;
    MIDIChannel m_channel;
    MIDIKey m_key;
    uint8_t m_velocity;
};

template<>
struct std::hash<NoteEvent::TransitionUnit>
{
    size_t operator()(NoteEvent::TransitionUnit const& tu) const
    {
        return (tu.channel << 8) | tu.key.code();
    }
};

inline bool operator==(NoteEvent::TransitionUnit const& l, NoteEvent::TransitionUnit const& r)
{
    return l.channel == r.channel && l.key == r.key;
}

class ControlChangeEvent : public Event
{
public:
    // Appendix 1.2 - Table of MIDI Controller Messages (Data Bytes)
    enum class Number : uint8_t
    {
        // 0x0 - 0x3f handled by LMControlChangeEvent
        DamperPedal = 0x40,
        Portamento,
        Sostenuto,
        SoftPedal,
        LegatoFootswitch,
        Hold2,
        SoundController1,
        SoundController2,
        SoundController3,
        SoundController4,
        SoundController5,
        SoundController6,
        SoundController7,
        SoundController8,
        SoundController9,
        SoundController10,
        GeneralPurposeController5,
        GeneralPurposeController6,
        GeneralPurposeController7,
        GeneralPurposeController8,
        PortamentoControl,
        Effects1Depth = 0x5b,
        Effects2Depth,
        Effects3Depth,
        Effects4Depth,
        Effects5Depth,
        DataEntryPlus1,
        DataEntryMinus1,
        NonRegisteredParameterNumberLSB,
        NonRegisteredParameterNumberMSB,
        RegisteredParameterNumberLSB,
        RegisteredParameterNumberMSB,
        AllSoundOff = 0x78,
        ResetAllControllers,
        LocalControlOnOff,
        AllNotesOff,
        OmniModeOff,
        OmniModeOn,
        PolyModeOn,
        PolyModeOnInclMono,
        Count
    };

    ControlChangeEvent(MIDIChannel channel, Number number, uint8_t value)
    : m_channel(channel), m_number(number), m_value(value) {}

    virtual void dump() const override
    {
        std::cerr << tick() << ": Control Change Event channel=" << (int)m_channel << ", number=" << std::hex << (int)m_number << std::dec << ", value=" << (int)m_value << std::endl;
    }

    virtual void execute(MIDIPlayer&) override
    { /* TODO */
    }

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        stream.put((uint8_t)(0xb0 + m_channel)); // Control Change
        stream.put((uint8_t)m_number);           // Number
        stream.put(m_value);
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<ControlChangeEvent>(*this); }

private:
    MIDIChannel m_channel;
    Number m_number;
    uint8_t m_value;
};

class ProgramChangeEvent : public Event
{
public:
    ProgramChangeEvent(MIDIChannel channel, uint8_t value)
    : m_channel(channel), m_value(value) {}

    virtual void dump() const override
    {
        std::cerr << "Program Change Event channel=" << (int)m_channel << ", data=" << std::hex << (int)m_value << std::dec << std::endl;
    }

    virtual void execute(MIDIPlayer&) override
    { /* TODO */
    }

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        stream.put((uint8_t)(0xc0 + m_channel)); // Program Change
        stream.put(m_value);
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<ProgramChangeEvent>(*this); }

private:
    MIDIChannel m_channel;
    uint8_t m_value;
};

class LMControlChangeEvent : public Event
{
public:
    enum class Number
    {
        BankSelect,
        ModulationWheel,
        BreathControl,
        FootController = 0x4,
        PortamentoTime,
        DataEntry,
        ChannelVolume,
        Balance,
        Pan = 0xa,
        ExpressionController,
        EffectControl1,
        EffectControl2,
        GeneralPurposeController1 = 0x10,
        GeneralPurposeController2,
        GeneralPurposeController3,
        GeneralPurposeController4,
    };

    enum class ByteIndex
    {
        MSB,
        LSB
    };

    LMControlChangeEvent(MIDIChannel channel, ByteIndex byte_index, Number number, uint8_t value)
    : m_channel(channel), m_byte_index(byte_index), m_number(number), m_value(value) {}

    virtual void dump() const override
    {
        std::cerr << "Control Change Event channel=" << (int)m_channel << ", number=" << std::hex << (int)m_number << std::dec
                  << "(" << (m_byte_index == ByteIndex::MSB ? "MSB" : "LSB") << "), value=" << (int)m_value << std::endl;
    }

    virtual void execute(MIDIPlayer&) override
    { /* TODO */
    }

    virtual bool is_serializable() const override { return true; }
    virtual void serialize(std::ostream& stream) const override
    {
        stream.put((uint8_t)(0xb0 + m_channel)); // Control Change
        stream.put((uint8_t)m_number | (m_byte_index == ByteIndex::MSB ? 0x20 : 0x0));
        stream.put(m_value);
    }

    virtual std::unique_ptr<Event> clone() const override { return std::make_unique<LMControlChangeEvent>(*this); }

private:
    MIDIChannel m_channel;
    ByteIndex m_byte_index;
    Number m_number;
    uint8_t m_value;
};
