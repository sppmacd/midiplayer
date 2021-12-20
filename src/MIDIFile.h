#include "MIDIInput.h"

// Based on https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf
class MIDIFile : public MIDIInput
{
public:
    MIDIFile(std::istream& in) { m_valid = read_midi(in); }

    virtual uint16_t ticks_per_quarter_note() const override { return m_ticks_per_quarter_note; }
    virtual bool is_valid() const override { return m_valid; }
    virtual void update(MIDIPlayer& player) override { m_tick += ticks_per_frame(player); }
    virtual size_t current_tick(MIDIPlayer const&) const override { return m_tick; }

    size_t ticks_per_frame(MIDIPlayer& player) const;

    void dump() const;

private:
    bool m_valid { false };
    bool m_header_encountered { false };

    enum class Format
    {
        SingleMultichannelTrack,
        SimultaneousTracks,
        SequentiallyIndependentTracks,
        Invalid
    };

    Format m_format { Format::Invalid };
    bool m_is_smpte = false;

    // For m_is_smpte = true
    uint8_t m_negative_smpte_format;
    uint8_t m_ticks_per_frame;

    // For m_is_smpte = false
    uint16_t m_ticks_per_quarter_note;

    size_t m_tick {};

    bool read_midi(std::istream& in);
    bool read_chunk(std::istream& in);
    bool read_header(std::istream& in);
    bool read_track_data(std::istream& in, size_t length);

    std::optional<uint32_t> read_variable_length_quantity(std::istream& in) const;
    std::unique_ptr<Event> read_meta_event(std::istream& in, uint8_t type);
};
