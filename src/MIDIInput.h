#pragma once

#include <istream>
#include <memory>
#include <optional>
#include <vector>

#include "Event.h"
#include "Track.h"

// Based on https://www.cs.cmu.edu/~music/cmsip/readings/Standard-MIDI-file-format-updated.pdf
class MIDIInput {
public:
    virtual ~MIDIInput() = default;

    void dump() const;
    virtual bool is_valid() const = 0;
    virtual uint16_t ticks_per_quarter_note() const = 0;
    virtual size_t current_tick(MIDIPlayer const&) const = 0;
    virtual std::optional<size_t> end_tick() const = 0;
    virtual void update(MIDIPlayer&) { }

    float ticks_per_second(MIDIPlayer const&) const;

    template<class Callback>
    void for_first_events_starting_from(size_t start, size_t max, Callback callback)
    {
        for (auto& track : m_tracks) {
            size_t counter = 0;
            auto& events = track.events();
            for (auto it = events.lower_bound(start); it != events.end(); it++) {
                callback(*it->second);
                counter++;
                if (counter >= max)
                    break;
            }
        }
    }

    template<class Callback>
    void for_first_events_starting_from_backwards(size_t start, size_t max, Callback callback)
    {
        for (auto& track : m_tracks) {
            size_t counter = 0;
            auto& events = track.events();
            for (auto it = events.upper_bound(start); it != events.begin();) {
                --it;
                callback(*it->second);
                counter++;
                if (counter >= max)
                    break;
            }
        }
    }

    template<class Callback>
    void for_each_event_in_time_order(Callback callback) const
    {
        size_t current_tick = 0;
        while (true) {
            size_t min_tick = 0;
            bool found = false;
            for (auto& track : m_tracks) {
                auto it = track.events().lower_bound(current_tick);
                if (it != track.events().end() && (!found || it->first < min_tick)) {
                    min_tick = it->first;
                    found = true;
                }
            }
            if (!found) {
                return;
            }
            for (auto& track : m_tracks) {
                auto events = track.events().equal_range(min_tick);
                for (auto it = events.first; it != events.second; it++) {
                    callback(*it->second);
                }
            }
            current_tick = min_tick + 1;
        }
    }

    template<class Callback>
    void for_each_track(Callback callback)
    {
        for (auto& track : m_tracks)
            callback(track);
    }

    std::vector<Event*> find_events_in_range(size_t start_tick, size_t end_tick) const;

    Track& track(size_t index) { return m_tracks[index]; }
    size_t track_count() const { return m_tracks.size(); }

protected:
    std::vector<Track> m_tracks;

    std::unique_ptr<Event> read_channeled_event(std::istream& in, uint8_t type, uint8_t channel);
    std::unique_ptr<Event> read_event(std::istream& in);
};
