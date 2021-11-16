#pragma once

#include <istream>
#include <memory>
#include <optional>
#include <vector>

#include "Event.h"
#include "Track.h"

// Based on https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf
class MIDI
{
public:
    void dump() const;
    virtual bool is_valid() const = 0;
    virtual uint16_t ticks_per_quarter_note() const = 0;
    virtual void update() {}

    template<class Callback>
    void for_each_event(Callback callback)
    {
        for(auto const& track: m_tracks)
        {
            for(auto const& event: track.events())
                callback(*event.second);
        }
    }

    template<class Callback>
    void for_each_event_backwards(Callback callback)
    {
        for(auto const& track: m_tracks)
        {
            auto& events = track.events();
            for(auto it = events.rbegin(); it != events.rend(); it++)
                callback(*it->second);
        }
    }
    std::vector<Event*> find_events_in_range(size_t start_tick, size_t end_tick) const;

protected:
    std::vector<Track> m_tracks;

    std::unique_ptr<Event> read_channeled_event(std::istream& in, uint8_t type, uint8_t channel);
    std::unique_ptr<Event> read_event(std::istream& in);
};
