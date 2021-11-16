#include "Track.h"

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
