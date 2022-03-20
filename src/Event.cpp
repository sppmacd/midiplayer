#include "Event.h"
#include "Config/Property.h"
#include "Logger.h"
#include "MIDIPlayer.h"
#include "RoundedEdgeRectangleShape.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <type_traits>

std::unique_ptr<Event> Event::create_event_from_name(std::string_view name)
{
    if(name == "Text")
        return std::make_unique<TextEvent>();
    return nullptr;
}

void EndOfTrackEvent::render(MIDIPlayer&, sf::RenderTarget& target)
{
    sf::RectangleShape rs({ target.getView().getSize().x, 5 });
    rs.setFillColor(sf::Color::White);
    rs.setPosition(0, tick());
    target.draw(rs);
}

void TextEvent::execute(MIDIPlayer& player)
{
    switch(m_type)
    {
        case Type::TrackName:
            player.display_label(MIDIPlayer::LabelType::TrackName, m_text, 180);
            break;
        case Type::Lyric:
        case Type::Text:
        case Type::Copyright:
        case Type::Instrument:
        case Type::Marker:
        case Type::CuePoint:
            // TODO
            break;
    }
}

Config::NamedFormalParameters TextEvent::formal_parameters() const
{
    return {
        { "type", Config::PropertyFormalParameter(Config::PropertyType::String, "type") },
        { "text", Config::PropertyFormalParameter(Config::PropertyType::String, "text") },
    };
}

bool TextEvent::read_from_parameters(Config::NamedParameters const& params)
{
    auto type = params.find("type");
    if(type != params.end())
    {
        if(type->first != "track_name")
        {
            logger::error("The only supported TextEvent type is 'track_name'");
            return false;
        }
    }
    auto text = params.find("text");
    if(text == params.end())
    {
        logger::error("required argument: 'text'");
        return false;
    }
    m_text = text->second.as_string();
    m_type = Type::TrackName;
    return true;
}

void SetTempoEvent::execute(MIDIPlayer& player)
{
    player.set_tempo(m_microseconds_per_quarter_note);
}

void NoteEvent::render(MIDIPlayer& player, sf::RenderTarget& target)
{
    // FIXME: Bring back color caching
    auto size = target.getView().getSize();
    const float scale = player.scale();

    auto is_visible = [&](float y_start, float y_size)
    {
        float real_y_start = (y_start + (player.real_time() ? -static_cast<int64_t>(player.current_tick()) : static_cast<int64_t>(player.current_tick()))) * scale;
        return !(real_y_start > 0 || real_y_start + y_size < -size.y);
    };

    auto render_note = [&](float y_start, float y_size, sf::Color color)
    {
        float real_y_start = (y_start + (player.real_time() ? -static_cast<int64_t>(player.current_tick()) : static_cast<int64_t>(player.current_tick()))) * scale;
        auto black = m_key.is_black();
        float key_size = black ? 0.5 : 1;
        RoundedEdgeRectangleShape rs({ key_size * size.x / MIDIPlayer::view_size_x, y_size }, 0.2f);
        rs.setFillColor(color);
        auto key_position = m_key.to_piano_position();
        rs.setPosition(key_position * size.x / MIDIPlayer::view_size_x, real_y_start);
        auto& shader = player.note_shader();
        shader.setUniform("uKeySize", sf::Vector2f { key_size * target.getSize().x / MIDIPlayer::view_size_x, y_size * (target.getSize().y / size.y) });
        shader.setUniform("uKeyPos", sf::Vector2f { key_position * target.getSize().x / MIDIPlayer::view_size_x, y_start * (target.getSize().y / size.y) });
        shader.setUniform("uIsBlack", black);
        target.draw(rs, sf::RenderStates { &shader });
    };

    if(!player.real_time())
    {
        auto start_note = player.started_notes().find(m_key);

        if(m_type == Type::Off)
        {
            if(start_note != player.started_notes().end())
            {
                int note_size_y = tick() - start_note->second.tick();
                auto y_start = size.y * scale - static_cast<int>(tick());
                auto y_size = note_size_y * scale;
                if(is_visible(y_start, y_size))
                {
                    auto color = player.resolve_color(*this);
                    if(player.current_tick() > start_note->second.tick() && player.current_tick() < tick())
                        player.spawn_random_particles(target, m_key, color, start_note->second.velocity());
                    render_note(y_start, y_size, color);
                }
                player.started_notes().erase(start_note);
            }
        }
        else if(start_note == player.started_notes().end())
            player.started_notes().insert({ m_key, *this });
    }
    else
    {
        auto end_note = player.ended_notes().find(m_key);
        if(m_type == Type::On)
        {
            auto note_size_y = static_cast<float>(player.current_tick() - tick());
            if(end_note != player.ended_notes().end() && end_note->second.has_value())
                note_size_y = std::min(static_cast<float>(end_note->second.value().tick() - tick()), note_size_y);
            auto y_start = tick();
            auto y_size = note_size_y * scale;
            if(is_visible(y_start, y_size))
            {
                auto color = player.resolve_color(*this);
                render_note(tick(), note_size_y * scale, color);
                if(end_note == player.ended_notes().end() || !end_note->second.has_value())
                    player.spawn_random_particles(target, m_key, color, velocity());
            }
            if(end_note != player.ended_notes().end())
                player.ended_notes().erase(end_note);
        }
        else if(end_note == player.ended_notes().end())
            player.ended_notes().insert({ m_key, *this });
    }
}

void NoteEvent::execute(MIDIPlayer& player)
{
    player.set_sound_playing(m_key, m_velocity, m_type == Type::On ? true : false, player.resolve_color(*this));
}
