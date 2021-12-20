#pragma once

#include "Event.h"
#include "Selector.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>
#include <SFML/Graphics.hpp>

class MIDI;

struct Particle
{
    sf::Vector2f position;
    sf::Vector2f motion;
    sf::Color color;
    int lifetime;
    int start_lifetime;
};

class MIDIPlayer
{
public:
    enum class RealTime
    {
        Yes,
        No
    };

     // TODO: Make these configurable??
    static constexpr float view_offset_x = 12.0;
    static constexpr float view_size_x = 52.0;
    static constexpr float piano_size_px = 200.f;

    MIDIPlayer(MIDI& midi, RealTime real_time);

    void set_fps(unsigned fps) { m_fps = fps; }
    void set_tempo(uint32_t microseconds_per_quarter_note) { m_microseconds_per_quarter_note = microseconds_per_quarter_note; }
    void stop() { m_playing = false; }

    void set_sound_playing(int index, int velocity, bool playing, sf::Color color);

    void update();

    bool playing() const { return m_playing; }
    void set_playing(bool playing) { m_playing = playing; }
    RealTime real_time() const { return m_real_time; }
    size_t current_tick() const;
    auto start_time() const { return m_start_time; }
    auto microseconds_per_quarter_note() const { return m_microseconds_per_quarter_note; }

    void spawn_particle(Particle&& p) { m_particles.push_back(std::move(p)); }
    void render_particles(sf::RenderTarget& target) const;
    void render_piano(sf::RenderTarget& target) const;
    void render_background(sf::RenderTarget& target) const;

    enum class Preview { Yes, No };

    void render_debug_info(sf::RenderTarget& target, Preview preview, sf::Time last_fps_time) const;

    sf::Shader& note_shader() const { return m_note_shader; }
    sf::Shader& particle_shader() const { return m_particle_shader; }

    std::map<MIDIKey, size_t>& started_notes() { return m_started_notes; }
    std::map<MIDIKey, std::optional<size_t>>& ended_notes() { return m_ended_notes; }

    sf::Color resolve_color(NoteEvent& event) const;
    int particle_count() const { return m_particle_count; }
    double scale() const { return real_time() == MIDIPlayer::RealTime::Yes ? m_real_time_scale : m_play_scale; }

private:
    static void ensure_sounds_generated();

    MIDI& m_midi;
    uint32_t m_microseconds_per_quarter_note { 500000 }; // 120 BPM
    unsigned m_fps { 60 };
    size_t m_current_tick { 0 };
    bool m_playing { true };
    struct Wind {
        double speed = 0;
        double target_speed = 0;
        sf::Vector2f pos;
        int time = 0;
        int start_time = 0;
    };
    
    std::list<Wind> m_winds;
    RealTime m_real_time { RealTime::No };
    std::list<Particle> m_particles;
    mutable sf::Shader m_note_shader;
    mutable sf::Shader m_particle_shader;
    sf::Font m_font;

    std::map<MIDIKey, size_t> m_started_notes;
    std::map<MIDIKey, std::optional<size_t>> m_ended_notes;

    std::vector<std::pair<std::vector<std::unique_ptr<Selector>>, sf::Color>> m_channel_colors;
    sf::Color m_default_color { 128, 128, 128, 128 };
    int m_particle_count = 2;
    float m_particle_radius = 0.5;
    float m_particle_glow_size = 0.1;
    size_t m_max_events_per_track = 4096;
    double m_real_time_scale = 0.05;
    double m_play_scale = 0.05;
    sf::Texture m_background_texture;
    sf::Sprite m_background_sprite;
    
    std::chrono::time_point<std::chrono::system_clock> m_start_time;
};
