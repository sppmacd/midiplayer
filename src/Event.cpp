#include "Event.h"
#include "MIDIPlayer.h"
#include "RoundedEdgeRectangleShape.hpp"
#include <cstddef>
#include <optional>
#include <random>
#include <type_traits>

void EndOfTrackEvent::execute(MIDIPlayer& player)
{
    player.stop();
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

void SetTempoEvent::execute(MIDIPlayer& player)
{
    player.set_tempo(m_microseconds_per_quarter_note);
}

void NoteEvent::render(MIDIPlayer& player, sf::RenderTarget& target)
{
    if(!m_cached_color.has_value())
        m_cached_color = player.resolve_color(*this);
    auto color = m_cached_color.value();
    auto size = target.getView().getSize();
    const float scale = player.scale();

    auto render_note = [&](float y_start, float y_size, sf::Color const& color)
    {
        float real_y_start = (y_start + (player.real_time() ? -static_cast<int64_t>(player.current_tick()) : static_cast<int64_t>(player.current_tick()))) * scale;

        if(real_y_start > 0 || real_y_start + y_size < -size.y)
            return;
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

    auto spawn_particles = [&](int velocity)
    {
        static std::default_random_engine engine;
        for(int i = 0; i < player.particle_count(); i++)
        {
            float velocity_factor = (velocity - 64) / 800.f + 0.03f;
            float rand_x_speed = (std::binomial_distribution<int>(100, 0.5)(engine) - 50) / 250.0;
            float rand_y_speed = -std::binomial_distribution<int>(100, 0.1)(engine) / 500.0 - velocity_factor;
            float offset = std::uniform_real_distribution<float>(-0.2, 0.2)(engine);
            int lifetime = std::gamma_distribution<double>(120, 0.9)(engine);
            player.spawn_particle(Particle {
                { m_key.to_piano_position() * size.x / MIDIPlayer::view_size_x + (m_key.is_black() ? 0.25f : 0.5f) + offset, 0 },
                { rand_x_speed, rand_y_speed },
                sf::Color(
                    std::min(255, color.r + 50),
                    std::min(255, color.g + 50),
                    std::min(255, color.b + 50)),
                lifetime, lifetime });
        }
    };

    if(!player.real_time())
    {
        auto start_note = player.started_notes().find(m_key);

        if(m_type == Type::Off)
        {
            if(start_note != player.started_notes().end())
            {
                int note_size_y = tick() - start_note->second.tick();
                if(player.current_tick() > start_note->second.tick() && player.current_tick() < tick())
                    spawn_particles(start_note->second.velocity());
                render_note((size.y * scale - static_cast<int>(tick())), note_size_y * scale, color);
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
            render_note(tick(), note_size_y * scale, color);
            if(end_note == player.ended_notes().end() || !end_note->second.has_value())
                spawn_particles(velocity());
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
