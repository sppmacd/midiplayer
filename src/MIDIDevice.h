#pragma once

#include "MIDIInput.h"
#include <atomic>
#include <fstream>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>

class MIDIDevice : public MIDIInput
{
public:
    MIDIDevice(std::string const& path);

    virtual bool is_valid() const override { return m_valid.load(std::memory_order_relaxed); }
    std::unique_ptr<Event> read_event();
    virtual void update(MIDIPlayer&) override;

    virtual uint16_t ticks_per_quarter_note() const override { return 192; }
    virtual size_t current_tick(MIDIPlayer const&) const override;

private:
    std::jthread m_io_thread;
    std::queue<std::unique_ptr<Event>> m_event_queue;
    std::mutex m_event_queue_mutex;
    std::atomic<bool> m_valid;
    std::atomic<MIDIPlayer*> m_player {nullptr};
};
