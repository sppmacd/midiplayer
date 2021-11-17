#include "Event.h"
#include "MIDIPlayer.h"
#include <cstddef>
#include <random>
#include <optional>
#include <type_traits>

void EndOfTrackEvent::execute(MIDIPlayer& player)
{
    player.stop();
}

void EndOfTrackEvent::render(MIDIPlayer&, sf::RenderTarget& target)
{
    sf::RectangleShape rs({target.getView().getSize().x, 5});
    rs.setFillColor(sf::Color::White);
    rs.setPosition(0, tick());
    target.draw(rs);
}

void SetTempoEvent::execute(MIDIPlayer& player)
{
    player.set_tempo(m_microseconds_per_quarter_note);
}

static std::map<MIDIKey, std::optional<size_t>> s_ended_notes;

static inline bool is_black(MIDIKey key)
{
    int relative_key = key % 12;
    return (relative_key == 1 || relative_key == 3 || relative_key == 6 || relative_key == 8 || relative_key == 10);
}

static inline float key_to_piano_position(MIDIKey key)
{
    int relative_key = key % 12;
    int octave = key / 12;
    if(is_black(key))
        return key_to_piano_position(key - 1) + 0.75f;
    if(relative_key < 1)
        return relative_key + octave * 7;
    if(relative_key < 3)
        return relative_key - 1 + octave * 7;
    if(relative_key < 6)
        return relative_key - 2 + octave * 7;
    if(relative_key < 8)
        return relative_key - 3 + octave * 7;
    if(relative_key < 10)
        return relative_key - 4 + octave * 7;
    return relative_key - 5 + octave * 7;
}

void NoteEvent::render(MIDIPlayer& player, sf::RenderTarget& target)
{
    static const sf::Color CHANNEL_COLORS[] {
        {0, 220, 0, 128},
        {128, 128, 220, 128},
        {220, 0, 0, 128},
        {220, 128, 0, 128},
        {220, 0, 128, 128},
        {0, 220, 220, 128},
    };
    
    auto color = m_channel < 6 ? CHANNEL_COLORS[m_channel] : sf::Color(128, 128, 128);
    auto size = target.getView().getSize();
    const float scale = player.real_time() == MIDIPlayer::RealTime::Yes ? 1 : 0.05;

    auto render_note = [&](float y_start, float y_size, sf::Color const& color) {
        float real_y_start = (y_start + (player.real_time() == MIDIPlayer::RealTime::Yes
            ? -static_cast<int64_t>(player.current_tick())
            : static_cast<int64_t>(player.current_tick()))) * scale;
        
        if(real_y_start > 0 || real_y_start + y_size < -size.y)
            return;
        auto black = is_black(m_key);
        float key_size = black ? 0.5 : 1;
        sf::RectangleShape rs({key_size * size.x / 128, y_size});
        rs.setFillColor(color);
        auto key_position = key_to_piano_position(m_key);
        rs.setPosition(key_position * size.x / 128, real_y_start);
        auto& shader = player.note_shader();
        shader.setUniform("uKeySize", key_size * target.getSize().x / 128.f);
        shader.setUniform("uIsBlack", black);
        target.draw(rs, sf::RenderStates{&shader});
    };

    auto spawn_particles = [&]() {
        static std::default_random_engine engine;
        for(int i = 0; i < 3; i++)
        {
            float rand_x_speed = std::uniform_real_distribution<float>(-0.4, 0.4)(engine);
            float rand_y_speed = std::uniform_real_distribution<float>(-0.1, -0.3)(engine);
            int lifetime = std::uniform_int_distribution<int>(90, 120)(engine);
            player.spawn_particle(Particle{{key_to_piano_position(m_key) * size.x / 128 + 0.5f, 0}, {rand_x_speed, rand_y_speed}, sf::Color(
                std::min(255, color.r + 50),
                std::min(255, color.g + 50),
                std::min(255, color.b + 50)),
                lifetime, lifetime});
        }
    };

    if(player.real_time() == MIDIPlayer::RealTime::No)
    {
        static std::map<MIDIKey, size_t> s_started_notes;
        auto start_note = s_started_notes.find(m_key);

        if(m_type == Type::Off)
        {
            if(start_note != s_started_notes.end())
            {
                int note_size_y = tick() - start_note->second;
                if(player.current_tick() > start_note->second && player.current_tick() < tick())
                    spawn_particles();
                render_note((size.y - static_cast<int>(tick())), note_size_y * scale, color);
                s_started_notes.erase(start_note);
            }
        }
        else if(start_note == s_started_notes.end())
            s_started_notes.insert({m_key, tick()});
    }
    else
    {
        auto end_note = s_ended_notes.find(m_key);
        if(m_type == Type::On)
        {
            auto note_size_y = static_cast<float>(player.current_tick() - tick());
            if(end_note != s_ended_notes.end() && end_note->second.has_value())
                note_size_y = std::min(static_cast<float>(end_note->second.value() - tick()), note_size_y);
            render_note(tick(), note_size_y, color);
            s_ended_notes.erase(m_key);

            if(player.current_tick() < end_note->second && player.current_tick() > tick())
                spawn_particles();
        }
        else if(end_note == s_ended_notes.end())
            s_ended_notes.insert({m_key, tick()});
    }
}

void NoteEvent::execute(MIDIPlayer& player)
{
    player.set_sound_playing(m_key, m_velocity, m_type == Type::On ? true : false);
}
