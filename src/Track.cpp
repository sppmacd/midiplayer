#include "Track.h"

void Track::add_event(std::unique_ptr<Event>&& data)
{
    m_events.insert({data->tick(), std::move(data)});

    if(m_max_events > 0 && m_events.size() > m_max_events)
        m_events.erase(m_events.begin());
}

std::vector<Event*> Track::find_events_in_range(size_t start_tick, size_t end_tick) const
{
    std::vector<Event*> out;
    for(size_t s = start_tick; s < end_tick; s++)
    {
        auto range = m_events.equal_range(s);
        std::for_each(range.first, range.second, [&](auto const& value) {
            out.push_back(value.second.get());
        });
    }
    return out;
}
