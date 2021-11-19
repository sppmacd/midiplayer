#pragma once

#include "Event.h"

#include <memory>
#include <vector>

class Track
{
public:
    void add_event(std::unique_ptr<Event>&& data);
    std::vector<Event*> find_events_in_range(size_t start_tick, size_t end_tick) const;
    std::multimap<size_t, std::unique_ptr<Event>> const& events() const { return m_events; }

    void set_max_events(size_t max) { m_max_events = max; }

private:
    std::multimap<size_t, std::unique_ptr<Event>> m_events;
    size_t m_max_events = 0;
};
