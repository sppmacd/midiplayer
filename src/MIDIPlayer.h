#pragma once

#include "Event.h"
#include "FileWatcher.h"
#include "MIDIOutput.h"
#include "MIDIPlayerConfig.h"
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
    ~MIDIPlayer();

    // Initialize the MIDIPlayer object: open MIDI devices/files.
    bool initialize(RealTime real_time, std::unique_ptr<MIDIInput>&& input, std::unique_ptr<MIDIOutput>&& output);

    bool is_initialized() const { return m_initialized; }

    // Do setup: load resources, do all heavy OpenGL initialization, reset MIDI output.
    void setup();

    void start_timer();

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
    bool current_time_is(Config::Time, size_t offset_in_frames) const;
    bool is_in_interval_frame(Config::Time, size_t offset_in_frames) const;
    size_t frame_count_for_time(Config::Time time) const;
    auto start_time() const { return m_start_time; }
    auto microseconds_per_quarter_note() const { return m_microseconds_per_quarter_note; }
    bool is_in_loop() const { return m_in_loop; }

    void spawn_particle(Particle&& p) { m_particles.push_back(std::move(p)); }
    void spawn_random_particles(sf::RenderTarget& target, MIDIKey key, sf::Color color, int velocity);

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

    sf::Shader& note_shader() const { return m_render_resources->note_shader; }
    sf::Shader& particle_shader() const { return m_render_resources->particle_shader; }

    std::map<MIDIKey, NoteEvent>& started_notes() { return m_started_notes; }
    std::map<MIDIKey, std::optional<NoteEvent>>& ended_notes() { return m_ended_notes; }

    int particle_count() const { return m_config.particle_count(); }
    double scale() const { return m_config.scale(); }
    sf::Color resolve_color(NoteEvent& event) const { return m_config.resolve_color(*this, event); }

    bool load_config_file(std::string const& path);
    void print_config_help() const;
    MIDIPlayerConfig const& config() const { return m_config; }

    MIDIInput* midi_input() { return m_midi_input.get(); }
    MIDIOutput* midi_output() { return m_midi_output.get(); }

    sf::Texture* get_background_image(std::string const& filename);

private:
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
    bool m_in_loop { false };

    struct Wind
    {
        double speed = 0;
        double target_speed = 0;
        sf::Vector2f pos;
        int time = 0;
        int start_time = 0;
    };

    struct Note
    {
        bool is_played { false };
        sf::Color color;
    };

    std::array<Note, 128> m_notes;
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

    // This is moved out of MIDIPlayer to shorten startup delay.
    // FIXME: Should be probably moved to separate Renderer class.
    struct RenderResources
    {
        mutable sf::Shader gradient_shader;
        mutable sf::Shader note_shader;
        mutable sf::Shader particle_shader;
        sf::Font display_font;
        sf::Font debug_font;
        sf::Texture particle_texture;
        std::map<std::string, sf::Texture> background_textures;
    };

    std::unique_ptr<RenderResources> m_render_resources;

    MIDIPlayerConfig m_config { *this };
    std::string m_config_file_path;
    FileWatcher m_config_file_watcher;

    std::map<MIDIKey, NoteEvent> m_started_notes;
    std::map<MIDIKey, std::optional<NoteEvent>> m_ended_notes;

    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::unique_ptr<MIDIInput> m_midi_input;
    std::unique_ptr<MIDIOutput> m_midi_output;
};
