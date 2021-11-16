#pragma once

#include "MIDI.h"
#include <atomic>
#include <fstream>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>

class MIDIDevice : public MIDI
{
public:
    MIDIDevice(std::string const& path);

    virtual bool is_valid() const override { return m_valid.load(std::memory_order_relaxed); }
    std::unique_ptr<Event> read_event();
    virtual void update() override;

    // TODO: Don't assume tempo 120 BPM with 60 ticks/s
    virtual uint16_t ticks_per_quarter_note() const override { return 30; }

private:
    std::jthread m_io_thread;
    std::queue<std::unique_ptr<Event>> m_event_queue;
    std::mutex m_event_queue_mutex;
    std::atomic<bool> m_valid;
    size_t m_tick = 0;
};
