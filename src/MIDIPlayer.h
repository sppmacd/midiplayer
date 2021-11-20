#pragma once

#include "Event.h"
#include "Selector.h"
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

    MIDIPlayer(MIDI& midi, RealTime real_time);

    void set_fps(unsigned fps) { m_fps = fps; }
    void set_tempo(uint32_t microseconds_per_quarter_note) { m_microseconds_per_quarter_note = microseconds_per_quarter_note; }
    void stop() { m_playing = false; }

    void set_sound_playing(int index, int velocity, bool playing);

    void update();

    bool playing() const { return m_playing; }
    RealTime real_time() const { return m_real_time; }
    size_t current_tick() const { return m_current_tick; }
    size_t ticks_per_frame() const;

    void spawn_particle(Particle&& p) { m_particles.push_back(std::move(p)); }
    void render_particles(sf::RenderTarget& target) const;
    void render_piano(sf::RenderTarget& target) const;

    sf::Shader& note_shader() const { return m_note_shader; }
    sf::Shader& particle_shader() const { return m_particle_shader; }

    std::map<MIDIKey, size_t>& started_notes() { return m_started_notes; }
    std::map<MIDIKey, std::optional<size_t>>& ended_notes() { return m_ended_notes; }

    sf::Color resolve_color(NoteEvent& event) const;
    int particle_count() const { return m_particle_count; }

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
    } m_wind;
    RealTime m_real_time { RealTime::No };
    std::list<Particle> m_particles;
    mutable sf::Shader m_note_shader;
    mutable sf::Shader m_particle_shader;

    std::map<MIDIKey, size_t> m_started_notes;
    std::map<MIDIKey, std::optional<size_t>> m_ended_notes;

    std::vector<std::pair<std::vector<std::unique_ptr<Selector>>, sf::Color>> m_channel_colors;
    sf::Color m_default_color { 128, 128, 128, 128 };
    int m_particle_count = 3;
    float m_particle_radius = 0.2;
    float m_particle_glow_size = 0.2;
    size_t m_max_events_per_track = 4096;
};
