#pragma once

#include "Event.h"

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

struct Tile {
    size_t start_tick;
    std::optional<size_t> end_tick;
    NoteEvent::TransitionUnit transition_unit;

    void dump() const;
};

class TileWorld {
public:
    // For Realtime mode
    void push_note_event(NoteEvent const& event);
    void dump() const;
    void render(sf::RenderTarget&, MIDIPlayer const&) const;

private:
    // List of tiles that haver no end tick set yet.
    std::unordered_map<NoteEvent::TransitionUnit, std::vector<Tile*>> m_pending_tiles;
    std::list<Tile> m_tiles;
};
