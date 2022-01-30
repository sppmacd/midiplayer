#pragma once

#include "ConfigFileReader.h"
#include "Event.h"
#include "FileWatcher.h"
#include "MIDIOutput.h"
#include "Selector.h"
#include <SFML/Graphics.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>

class MIDIInput;
struct Particle
{
    sf::Vector2f position;
    sf::Vector2f motion;
    sf::Color color;
    int lifetime;
    int start_lifetime;
};

template<>
struct std::hash<sf::Color>
{
    std::size_t operator()(sf::Color const& s) const noexcept
    {
        return s.r ^ (s.g << 1) ^ (s.b << 2) ^ (s.a << 3);
    }
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

    MIDIPlayer();
    void initialize(RealTime real_time, std::unique_ptr<MIDIInput>&& input, std::unique_ptr<MIDIOutput>&& output);
    bool is_initialized() const { return m_initialized; }

    void set_fps(unsigned fps) { m_fps = fps; }
    unsigned fps() const { return m_fps; }
    void set_tempo(uint32_t microseconds_per_quarter_note) { m_microseconds_per_quarter_note = microseconds_per_quarter_note; }
    void set_sound_playing(int index, int velocity, bool playing, sf::Color color);
    void stop() { m_playing = false; }

    void update();

    bool playing() const { return m_playing; }
    void set_playing(bool playing) { m_playing = playing; }
    bool real_time() const { return m_real_time; }
    size_t current_tick() const;
    size_t current_frame() const { return m_current_frame; }
    auto start_time() const { return m_start_time; }
    auto microseconds_per_quarter_note() const { return m_microseconds_per_quarter_note; }

    void spawn_particle(Particle&& p) { m_particles.push_back(std::move(p)); }

    enum class LabelType
    {
        TrackName
    };
    void display_label(LabelType, std::string text, int duration);

    struct DebugInfo
    {
        bool full_info;
        sf::Time last_fps_time;
    };

    void render(sf::RenderTarget& target, DebugInfo const& debug_info);

    sf::Shader& note_shader() const { return m_note_shader; }
    sf::Shader& particle_shader() const { return m_particle_shader; }

    std::map<MIDIKey, NoteEvent>& started_notes() { return m_started_notes; }
    std::map<MIDIKey, std::optional<NoteEvent>>& ended_notes() { return m_ended_notes; }

    sf::Color resolve_color(NoteEvent& event) const;
    int particle_count() const { return m_particle_count; }
    double scale() const { return real_time() ? m_real_time_scale : m_play_scale; }

    bool load_config_file(std::string const& path);
    void print_config_help() const;

private:
    static void ensure_sounds_generated();
    void generate_particle_texture();

    void render_particles(sf::RenderTarget& target) const;
    void render_overlay(sf::RenderTarget& target) const;
    void render_background(sf::RenderTarget& target) const;
    void render_debug_info(sf::RenderTarget& target, DebugInfo const& debug_info) const;

    bool reload_config_file();

    uint32_t m_microseconds_per_quarter_note { 500000 }; // 120 BPM
    unsigned m_fps { 60 };
    size_t m_current_tick { 0 };
    size_t m_current_frame { 0 };
    bool m_playing { true };
    bool m_initialized { false };

    struct Wind
    {
        double speed = 0;
        double target_speed = 0;
        sf::Vector2f pos;
        int time = 0;
        int start_time = 0;
    };

    std::list<Wind> m_winds;
    bool m_real_time { false };
    std::list<Particle> m_particles;

    struct Label
    {
        LabelType type;
        std::string text;
        int remaining_duration;
        int total_duration;
    };

    std::list<Label> m_labels;

    mutable sf::Shader m_gradient_shader;
    mutable sf::Shader m_note_shader;
    mutable sf::Shader m_particle_shader;
    sf::Font m_debug_font;
    sf::Font m_display_font;
    std::string m_config_file_path;
    ConfigFileReader m_config_file_reader;
    FileWatcher m_config_file_watcher;

    std::map<MIDIKey, NoteEvent> m_started_notes;
    std::map<MIDIKey, std::optional<NoteEvent>> m_ended_notes;

    std::vector<std::pair<std::vector<std::unique_ptr<Selector>>, sf::Color>> m_channel_colors;
    sf::Color m_default_color { 100, 100, 255 };
    sf::Color m_background_color { 10, 10, 10 };
    sf::Color m_overlay_color { 5, 5, 5 };
    int m_particle_count = 2;
    float m_particle_radius = 0.5;
    float m_particle_glow_size = 0.1;
    size_t m_max_events_per_track = 4096;
    double m_real_time_scale = 0.05;
    double m_play_scale = 0.05;
    int m_label_font_size = 50;
    int m_label_fade_time = 30;
    sf::Texture m_background_texture;
    sf::Sprite m_background_sprite;
    sf::Texture m_particle_texture;

    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::unique_ptr<MIDIInput> m_midi_input;
    std::unique_ptr<MIDIOutput> m_midi_output;
};
