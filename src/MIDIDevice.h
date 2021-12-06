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

    virtual uint16_t ticks_per_quarter_note() const override { return 192; }
    virtual void on_player_attach(MIDIPlayer& player) override { m_player = &player; }

private:
    std::jthread m_io_thread;
    std::queue<std::unique_ptr<Event>> m_event_queue;
    std::mutex m_event_queue_mutex;
    std::atomic<bool> m_valid;
    MIDIPlayer* m_player {nullptr};
};
