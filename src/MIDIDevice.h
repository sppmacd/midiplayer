#pragma once

#include "MIDIInput.h"
#include <atomic>
#include <fstream>
#include <memory>
#include <queue>
#include <stop_token>
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
    virtual size_t end_tick() const override { return 0; }

private:
    std::jthread m_io_thread;
    std::stop_source m_io_thread_stop_source;
    std::queue<std::unique_ptr<Event>> m_event_queue;
    std::mutex m_event_queue_mutex;
    std::atomic<bool> m_valid;
    std::atomic<MIDIPlayer*> m_player {nullptr};
};
