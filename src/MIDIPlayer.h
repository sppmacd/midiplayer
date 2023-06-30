#pragma once

#include "Config/Property.h"
#include "Event.h"
#include "FileWatcher.h"
#include "MIDIOutput.h"
#include "MIDIPlayerConfig.h"
#include "Pedals.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>

class MIDIInput;
struct Particle {
    sf::Vector2f position;
    sf::Vector2f motion;
    sf::Color color;
    float temperature;
};

template<>
struct std::hash<sf::Color> {
    std::size_t operator()(sf::Color const& s) const noexcept
    {
        return s.r ^ (s.g << 1) ^ (s.b << 2) ^ (s.a << 3);
    }
};

class MIDIPlayer {
public:
    enum class RealTime {
        Yes,
        No
    };

    // TODO: Make these configurable??
    static constexpr float view_offset_x = 12.0;
    static constexpr float view_size_x = 52.0;
    static constexpr float piano_size_px = 200.f;

    MIDIPlayer();
    ~MIDIPlayer();

    static MIDIPlayer& the();

    struct Args {
        enum class Mode {
            Realtime,
            Play
        };
        Mode mode {};
        bool print_config_help = false;
        bool render_to_stdout = false;
        bool should_render_debug_info_in_preview = false;
        bool force_overwrite = false;
        bool remove_file_if_nothing_written = false;
        std::string midi_output;
        std::string config_file_path;
        std::string marker_file_name;
    };
    void run(Args const& args);

    // Initialize the MIDIPlayer object: open MIDI devices/files.
    bool initialize(RealTime real_time, std::unique_ptr<MIDIInput>&& input, std::unique_ptr<MIDIOutput>&& output);

    bool is_initialized() const { return m_initialized; }

    // Do setup: load resources, do all heavy OpenGL initialization, reset MIDI output.
    void setup();

    void start_timer();

    void set_headless() { m_headless = true; }
    bool is_headless() const { return m_headless; }
    void set_fps(unsigned fps) { m_fps = fps; }
    unsigned fps() const { return m_fps; }
    void set_tempo(uint32_t microseconds_per_quarter_note) { m_microseconds_per_quarter_note = microseconds_per_quarter_note; }
    void set_sound_playing(int index, int velocity, bool playing, sf::Color color);
    void stop() { m_playing = false; }
    void set_paused(bool b) { m_paused = b; }
    bool is_paused() const { return m_paused; }

    void update();

    bool playing() const { return m_playing; }

    // NOTE: This is the only thread-safe thing here!
    void set_playing(bool playing) { m_playing = playing; }

    bool real_time() const { return m_real_time; }
    size_t current_frame() const { return m_current_frame; }
    size_t current_tick() const { return m_current_tick; }
    bool current_time_is(Config::Time, size_t offset_in_frames) const;
    bool is_in_interval_frame(Config::Time, size_t offset_in_frames) const;
    size_t frame_count_for_time(Config::Time time, size_t offset_in_frames) const;
    auto start_time() const { return m_start_time; }
    auto microseconds_per_quarter_note() const { return m_microseconds_per_quarter_note; }
    bool is_in_loop() const { return m_in_loop; }

    void spawn_particle(Particle&& p) { m_particles.push_back(std::move(p)); }
    void spawn_random_particles(sf::RenderTarget& target, MIDIKey key, sf::Color color, int velocity);

    enum class LabelType {
        TrackName
    };
    void display_label(LabelType, std::string text, int duration);

    struct DebugInfo {
        bool full_info;
        sf::Time last_fps_time;
    };

    void render(sf::RenderTarget& target, DebugInfo const& debug_info);

    sf::Shader& note_shader() const { return m_render_resources->note_shader; }
    sf::Shader& particle_shader() const { return m_render_resources->particle_shader; }

    using StartedNote = NoteEvent;
    std::array<std::optional<StartedNote>, 128>& started_notes() { return m_started_notes; }
    using EndedNote = std::optional<NoteEvent>;
    std::array<std::optional<EndedNote>, 128>& ended_notes() { return m_ended_notes; }

    int particle_count() const { return m_config.particle_count(); }
    double scale() const { return m_config.scale(); }
    sf::Color resolve_color(NoteEvent& event);
    void update_note_transitions(Config::SelectorList const& selectors, sf::Color color, double transition);
    void add_static_tile_color(Config::SelectorList const& selectors, sf::Color color);

    bool load_config_file(std::string const& path);
    void print_config_help() const;
    MIDIPlayerConfig const& config() const { return m_config; }

    MIDIInput* midi_input() { return m_midi_input.get(); }
    MIDIOutput* midi_output() { return m_midi_output.get(); }

    sf::Texture* get_background_image(std::string const& filename);
    std::string get_stats_string(bool full) const;

    void did_read_events(size_t count) { m_events_read += count; }
    auto& pedals() { return m_pedals; }
    auto& pedals() const { return m_pedals; }

private:
    void generate_particle_texture();
    void generate_minimap_texture();
    size_t calculate_current_tick() const;

    void render_particles(sf::RenderTarget& target) const;
    void render_overlay(sf::RenderTarget& target) const;
    void render_background(sf::RenderTarget& target) const;
    void render_debug_info(sf::RenderTarget& target, DebugInfo const& debug_info) const;
    void render_progress_bar(sf::RenderTarget& target) const;
    void render_pedals(sf::RenderTarget& target) const;

    bool reload_config_file();

    sf::Vector2f progress_bar_size(sf::Vector2f window_size) const
    {
        constexpr float height = 12.f;
        const float width = window_size.x * 1.f / 3;
        return { width, height };
    }

    uint32_t m_microseconds_per_quarter_note { 500000 }; // 120 BPM
    unsigned m_fps { 60 };
    size_t m_current_tick { 0 };
    size_t m_current_frame { 0 };
    std::atomic<bool> m_playing { true };
    bool m_paused = false;
    bool m_initialized { false };
    bool m_in_loop { false };
    bool m_headless { false };
    Pedals m_pedals;

    struct Wind {
        double speed = 0;
        double target_speed = 0;
        sf::Vector2f pos;
        int time = 0;
        int start_time = 0;
    };

    struct Note {
        bool is_played { false };
        sf::Color color;
    };

    std::array<Note, 128> m_notes;
    std::list<Wind> m_winds;
    bool m_real_time { false };
    std::list<Particle> m_particles;
    std::vector<std::pair<Config::SelectorList, sf::Color>> m_static_tile_colors;

    struct Label {
        LabelType type;
        std::string text;
        int remaining_duration;
        int total_duration;
    };

    std::list<Label> m_labels;

    // This is moved out of MIDIPlayer to shorten startup delay.
    // FIXME: Should be probably moved to separate Renderer class.
    struct RenderResources {
        mutable sf::Shader gradient_shader;
        mutable sf::Shader note_shader;
        mutable sf::Shader notelight_shader;
        mutable sf::Shader particle_shader;
        sf::Font display_font;
        sf::Font debug_font;
        sf::Texture particle_texture;
        sf::Texture minimap_texture;
        sf::Texture pedals_texture;
        std::map<std::string, sf::Texture> background_textures;
    };

    std::unique_ptr<RenderResources> m_render_resources;

    MIDIPlayerConfig m_config { *this };
    std::string m_config_file_path;
    FileWatcher m_config_file_watcher;

    std::array<std::optional<StartedNote>, 128> m_started_notes;
    std::array<std::optional<EndedNote>, 128> m_ended_notes;

    std::unordered_map<NoteEvent::TransitionUnit, Config::AnimatableProperty<sf::Color>> m_note_transitions;

    std::chrono::time_point<std::chrono::system_clock> m_start_time;
    std::unique_ptr<MIDIInput> m_midi_input;
    std::unique_ptr<MIDIOutput> m_midi_output;

    size_t m_events_executed = 0;
    size_t m_events_read = 0;
    size_t m_events_written = 0;
};
