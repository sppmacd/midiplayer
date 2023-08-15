#include "TileWorld.hpp"

#include "MIDIPlayer.h"
#include <SFML/Graphics/RenderStates.hpp>

void Tile::dump() const
{
    fmt::print("{}..{}: {} @C{}\n",
        start_tick,
        end_tick ? std::to_string(*end_tick) : "now",
        transition_unit.key.code(),
        transition_unit.channel);
}

void TileWorld::push_note_event(NoteEvent const& event)
{
    event.dump();
    auto transition_unit = event.transition_unit();
    switch (event.type()) {
        case NoteEvent::Type::On: {
            auto& tile = m_tiles.emplace_back(Tile { event.tick(), {}, transition_unit });
            m_pending_tiles[transition_unit].push_back(&tile);
        } break;
        case NoteEvent::Type::Off: {
            auto& tiles = m_pending_tiles[transition_unit];
            if (tiles.empty()) {
                fmt::print("NoteOff without NoteOn!\n");
                // This may happen when launching MIDIPlayer with a midi key pressed,
                // then depressing that key
                return;
            }
            tiles.back()->end_tick = event.tick();
            tiles.back()->dump();
            tiles.pop_back();
        } break;
    }
}

void TileWorld::dump() const
{
    fmt::print("{} events\n", m_tiles.size());
    for (auto const& tile : m_tiles) {
        tile.dump();
    }
}

void TileWorld::render(sf::RenderTarget& target, MIDIPlayer const& player) const
{
    float offset = player.current_tick();

    auto render_tile = [&](Tile const& tile) {
        float y_start = tile.start_tick;
        float y_end = tile.end_tick.value_or(offset + 10);
        if (player.real_time()) {
            y_start = y_start - offset;
            y_end = y_end - offset;
        } else {
            y_start = -(y_start - offset);
            y_end = -(y_end - offset);
        }
        // Avoid negative sizes
        if (y_start > y_end) {
            std::swap(y_start, y_end);
        }
        y_start *= player.scale();
        y_end *= player.scale();
        auto color = player.resolve_color(tile);

        float x_position = tile.transition_unit.key.to_piano_position();

        sf::Vector2f const extent { 1, 1 };
        bool black = tile.transition_unit.key.is_black();
        auto tile_size = sf::Vector2f { black ? 0.7f : 1, y_end - y_start };
        auto tile_position = sf::Vector2f { x_position - (black ? 0.15f : 0), y_start };

        sf::RectangleShape rect(tile_size + extent);
        rect.setPosition(tile_position - extent / 2.f);
        rect.setFillColor(color);

        constexpr float TileSpacing = 2;

        auto screen_tile_position = target.mapCoordsToPixel(tile_position);
        screen_tile_position.x += TileSpacing;
        screen_tile_position.y += TileSpacing;
        auto screen_tile_end_position = target.mapCoordsToPixel(tile_position + tile_size);
        screen_tile_end_position.x -= TileSpacing;
        screen_tile_end_position.y -= TileSpacing;

        sf::Vector2f screen_tile_size { screen_tile_end_position - screen_tile_position };

        auto& shader = player.note_shader();
        shader.setUniform("uKeySize", screen_tile_size);
        shader.setUniform("uKeyPos", sf::Vector2f { static_cast<float>(screen_tile_position.x), static_cast<float>(target.getSize().y - screen_tile_position.y - screen_tile_size.y) });
        shader.setUniform("uIsBlack", black);
        target.draw(rect, sf::RenderStates { &shader });
    };

    for (auto const& tile : m_tiles) {
        render_tile(tile);
    }
}
