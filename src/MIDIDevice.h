#pragma once

#include "MIDIInput.h"
#include "MIDIOutput.h"

#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <rtmidi/RtMidi.h>
#include <stop_token>
#include <thread>

class MIDIDeviceInput : public MIDIInput {
public:
    MIDIDeviceInput(int port);

    virtual bool is_valid() const override { return m_valid.load(std::memory_order_relaxed); }
    std::unique_ptr<Event> read_event();
    virtual void update(MIDIPlayer&) override;

    virtual uint16_t ticks_per_quarter_note() const override { return 192; }
    virtual size_t current_tick(MIDIPlayer const&) const override;
    virtual std::optional<size_t> end_tick() const override { return {}; }

private:
    std::queue<std::unique_ptr<Event>> m_event_queue;
    std::mutex m_event_queue_mutex;
    std::atomic<bool> m_valid;
    std::atomic<MIDIPlayer*> m_player { nullptr };
    RtMidiIn m_input;
};

class MIDIDeviceOutput : public MIDIOutput {
public:
    MIDIDeviceOutput(int port);

    virtual bool is_valid() const override { return m_valid; }
    virtual void write_event(Event const&) override;

private:
    RtMidiOut m_output;
    bool m_valid { false };
};

namespace MIDI {

void print_available_ports();

}
