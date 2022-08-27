#pragma once

#include "MIDIInput.h"
#include "MIDIOutput.h"

#include <fstream>

// Based on https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf

enum class MIDIFileFormat {
    SingleMultichannelTrack,
    SimultaneousTracks,
    SequentiallyIndependentTracks,
    Invalid
};
class MIDIFileInput : public MIDIInput {
public:
    MIDIFileInput(std::istream& in) { m_valid = read_midi(in); }

    virtual uint16_t ticks_per_quarter_note() const override { return m_ticks_per_quarter_note; }
    virtual bool is_valid() const override { return m_valid; }
    virtual void update(MIDIPlayer& player) override;
    virtual size_t current_tick(MIDIPlayer const&) const override { return m_tick; }
    virtual std::optional<size_t> end_tick() const override { return m_end_tick; }

    void dump() const;

private:
    bool m_valid { false };
    bool m_header_encountered { false };

    MIDIFileFormat m_format { MIDIFileFormat::Invalid };
    bool m_is_smpte = false;

    // For m_is_smpte = true
    uint8_t m_negative_smpte_format;
    uint8_t m_ticks_per_frame;

    // For m_is_smpte = false
    uint16_t m_ticks_per_quarter_note;

    double m_tick {};
    size_t m_end_tick {};

    uint8_t m_running_status = 0;

    bool read_midi(std::istream& in);
    bool read_chunk(std::istream& in);
    bool read_header(std::istream& in);
    bool read_track_data(std::istream& in, size_t length);

    std::optional<uint32_t> read_variable_length_quantity(std::istream& in) const;
    std::unique_ptr<Event> read_meta_event(std::istream& in, uint8_t type);
};

class MIDIFileOutput : public MIDIOutput {
public:
    explicit MIDIFileOutput(std::string path);
    ~MIDIFileOutput();

    virtual bool is_valid() const override { return !m_output.fail(); }
    virtual void write_event(Event const&) override;

private:
    std::ofstream m_output;
    size_t m_track_length_offset {};
    size_t m_last_tick {};
    uint32_t m_track_length {};

    void write_variable_length_quantity(uint32_t);
};
