#include "MIDIPlayer.h"

#include "Event.h"
#include "Logger.h"
#include "MIDIDevice.h"
#include "MIDIFile.h"
#include "Resources.h"
#include "RoundedEdgeRectangleShape.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <cassert>
#include <climits>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <random>
#include <ranges>
#include <signal.h>
#include <sstream>

using namespace std::literals;

constexpr float ParticleTemperatureMean = 120;

static MIDIPlayer* s_the = nullptr;

MIDIPlayer::MIDIPlayer()
{
    assert(!s_the);
    s_the = this;
}

MIDIPlayer::~MIDIPlayer()
{
    // Don't add bloat to MIDI files. These events are sent by device output anyway.
    if (!m_midi_output || !dynamic_cast<MIDIDeviceOutput*>(m_midi_output.get()))
        return;
    for (size_t s = 0; s < 16; s++) {
        ControlChangeEvent c1(s, ControlChangeEvent::Number::AllSoundOff, 0);
        m_midi_output->write_event(c1);
        ControlChangeEvent c2(s, ControlChangeEvent::Number::AllNotesOff, 0);
        m_midi_output->write_event(c2);
    }
    s_the = nullptr;
}

MIDIPlayer& MIDIPlayer::the()
{
    assert(s_the);
    return *s_the;
}

void MIDIPlayer::run(Args const& args)
{
    std::unique_ptr<sf::RenderTexture> render_texture = [&]() -> std::unique_ptr<sf::RenderTexture> {
        if (!is_headless() && args.render_to_stdout) {
            if (isatty(STDOUT_FILENO)) {
                logger::error("stdout is a terminal, refusing to print binary data");
                return nullptr;
            }

            auto texture = std::make_unique<sf::RenderTexture>();
            // TODO: Support custom resolution/FPS/format/...
            if (!texture->create(1920, 1080)) {
                logger::error("Failed to create render texture, ignoring");
                return nullptr;
            }
            logger::info("Rendering to stdout (RGBA24 1920x1080 60fps)");
            if (args.mode == Args::Mode::Realtime)
                logger::warning("Realtime mode is not recommended for rendering, consider recording it to MIDI file first and playing");
            return texture;
        }
        return nullptr;
    }();

    bool is_fullscreen = false;
    bool should_render_debug_info_in_preview = args.should_render_debug_info_in_preview;
    std::optional<sf::RenderWindow> window;

    auto create_windowed = [&]() {
        is_fullscreen = false;
        window.emplace(sf::VideoMode::getDesktopMode(), "MIDI Player", sf::Style::Default, sf::ContextSettings { 0, 0, 1 });
        if (!render_texture)
            window->setFramerateLimit(60);
        window->setMouseCursorVisible(true);
    };
    auto create_fullscreen = [&]() {
        is_fullscreen = true;
        window.emplace(sf::VideoMode::getDesktopMode(), "MIDI Player", sf::Style::Fullscreen, sf::ContextSettings { 0, 0, 1 });
        if (!render_texture)
            window->setFramerateLimit(60);
        window->setMouseCursorVisible(false);
    };
    if (!is_headless()) {
        create_windowed();
        sf::Image icon;
        icon.loadFromFile(find_resource_path() + "/icon32.png");
        window->setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }

    sf::Clock fps_clock;
    sf::Clock periodic_stats_clock;
    sf::Time last_fps_time;

    std::ofstream marker_file { args.marker_file_name, std::ios::app };
    if (!args.marker_file_name.empty() && marker_file.fail())
        logger::warning("Failed to open marker file '{}'. Markers will not be saved.", args.marker_file_name);
    marker_file << "# markers " << time(nullptr) << std::endl;
    auto write_marker = [&](std::string name) {
        if (args.marker_file_name.empty())
            return;
        logger::info("Adding marker {} at {} to {}", name, current_tick(), args.marker_file_name);
        marker_file << name << ": " << current_tick() << std::endl;
    };

    signal(SIGINT, [](int) {
        MIDIPlayer::the().set_playing(false);
    });
    signal(SIGTERM, [](int) {
        MIDIPlayer::the().set_playing(false);
    });

    start_timer();

    while (playing()) {
        if (!is_headless()) {
            sf::Event event;
            while (window->pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    set_playing(false);
                else if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::F11) {
                        if (is_fullscreen)
                            create_windowed();
                        else
                            create_fullscreen();
                    } else if (event.key.code == sf::Keyboard::F3) {
                        should_render_debug_info_in_preview = !should_render_debug_info_in_preview;
                    } else if (event.key.code >= sf::Keyboard::Num0 && event.key.code <= sf::Keyboard::Num9) {
                        write_marker(std::to_string(event.key.code - sf::Keyboard::Num0));
                    } else if (event.key.code >= sf::Keyboard::Numpad0 && event.key.code <= sf::Keyboard::Numpad9) {
                        write_marker(std::to_string(event.key.code - sf::Keyboard::Numpad0));
                    } else if (event.key.code == sf::Keyboard::Right) {
                        auto input = dynamic_cast<MIDIFileInput*>(midi_input());
                        if (input) {
                            input->move_forward(event.key.control);
                        }
                    } else if (event.key.code == sf::Keyboard::Space) {
                        set_paused(!is_paused());
                    }
                }
            }
        }

        update();
        if (!is_headless()) {
            // FIXME: Last FPS should be stored in MIDIPlayer somehow!
            render(*window, { .full_info = should_render_debug_info_in_preview, .last_fps_time = last_fps_time });
            window->display();

            if (render_texture) {
                render(*render_texture, { .full_info = false, .last_fps_time = last_fps_time });
                render_texture->display();
                auto image = render_texture->getTexture().copyToImage();

                // NOTE: RGBA only!
                fwrite(image.getPixelsPtr(), 4, image.getSize().x * image.getSize().y, stdout);
            }
        } else {
            sf::sleep(sf::seconds(1.f / fps()) - fps_clock.getElapsedTime());
        }
        last_fps_time = fps_clock.restart();

        if (periodic_stats_clock.getElapsedTime() > sf::seconds(1) && isatty(STDOUT_FILENO)) {
            periodic_stats_clock.restart();
            std::cout << get_stats_string(true) << std::endl;
        }
    }
    write_marker("end");
}

bool MIDIPlayer::initialize(RealTime real_time, std::unique_ptr<MIDIInput>&& input, std::unique_ptr<MIDIOutput>&& output)
{
    // FIXME: This would need real error propagation instead of this. Maybe just use exceptions.
    if (input && !input->is_valid()) {
        logger::error("Invalid MIDI input");
        return false;
    }
    if (output && !output->is_valid()) {
        logger::error("Invalid MIDI output");
        return false;
    }

    m_real_time = real_time == RealTime::Yes;
    m_midi_output = std::move(output);
    m_midi_input = std::move(input);

    m_initialized = true;
    return true;
}

void MIDIPlayer::setup()
{
    if (!m_headless) {
        m_render_resources = std::make_unique<RenderResources>();

        auto resource_path = find_resource_path();
        logger::info("Resource path: {}", resource_path);
        if (
            m_render_resources->gradient_shader.loadFromFile(resource_path + "/shaders/gradient.vert", resource_path + "/shaders/gradient.frag")
            && m_render_resources->note_shader.loadFromFile(resource_path + "/shaders/note.vert", resource_path + "/shaders/note.frag")
            && m_render_resources->notelight_shader.loadFromFile(resource_path + "/shaders/notelight.vert", resource_path + "/shaders/notelight.frag")
            && m_render_resources->particle_shader.loadFromFile(resource_path + "/shaders/particle.vert", resource_path + "/shaders/particle.frag")) {
            logger::info("Shaders loaded");
        } else {
            exit(1);
        }

        if (m_render_resources->debug_font.loadFromFile(resource_path + "/dejavu-sans-mono.ttf"))
            logger::info("Font loaded");

        if (m_render_resources->pedals_texture.loadFromFile(resource_path + "/pedals.png")
            && m_render_resources->smoke_texture.loadFromFile(resource_path + "/smoke.png")) {
            logger::info("Textures loaded");
        } else {
            exit(1);
        }
    }

    // Don't add bloat to MIDI files. These events are sent by device output anyway.
    if (!m_midi_output || !dynamic_cast<MIDIDeviceOutput*>(m_midi_output.get()))
        return;
    for (size_t s = 0; s < 16; s++) {
        ControlChangeEvent c1(s, ControlChangeEvent::Number::ResetAllControllers, 0);
        m_midi_output->write_event(c1);
    }
}

void MIDIPlayer::start_timer()
{
    m_start_time = std::chrono::system_clock::now();
    m_in_loop = true;
}

sf::Color MIDIPlayer::resolve_color(NoteEvent& event)
{
    for (auto const& selector_list : m_static_tile_colors) {
        for (auto const& selector : selector_list.first) {
            if (selector->matches(event.transition_unit(), &event))
                return selector_list.second;
        }
    }

    auto maybe_pending_transition = m_note_transitions.find(event.transition_unit());
    if (maybe_pending_transition != m_note_transitions.end())
        return maybe_pending_transition->second;

    return m_config.default_color();
}

void MIDIPlayer::update_note_transitions(Config::SelectorList const& selectors, sf::Color color, double transition)
{
    // FIXME: Very naive and not the best.
    for (uint8_t key = 0; key < 128; key++) {
        for (uint8_t channel = 0; channel < 16; channel++) {
            NoteEvent::TransitionUnit transition_unit { key, channel };
            bool matches = false;
            if (selectors.empty())
                matches = true;
            else {
                for (auto& selector : selectors) {
                    if (selector->matches(transition_unit, nullptr)) {
                        matches = true;
                        break;
                    }
                }
            }
            if (!matches)
                continue;

            auto maybe_pending_transition = m_note_transitions.find(transition_unit);
            if (maybe_pending_transition != m_note_transitions.end())
                maybe_pending_transition->second.set_value_with_factor(color, transition);
            Config::AnimatableProperty<sf::Color> color_animatable { std::move(color) };
            color_animatable.set_value_with_factor(color, transition);
            m_note_transitions.insert(std::make_pair(transition_unit, std::move(color_animatable)));
        }
    }
}

void MIDIPlayer::add_static_tile_color(Config::SelectorList const& selectors, sf::Color color)
{
    m_static_tile_colors.push_back({ selectors, color });
}

bool MIDIPlayer::load_config_file(std::string const& path)
{
    m_config_file_path = path;
    m_config_file_watcher = FileWatcher(path);
    return reload_config_file();
}

bool MIDIPlayer::reload_config_file()
{
    if (!m_headless)
        assert(m_render_resources);
    bool success = m_config.reload(m_config_file_path);

    if (!m_headless) {
        generate_dust_texture();
        generate_minimap_texture();

        if (m_config.display_font().empty()) {
            logger::warning("No display font is specified. Using debug font.");
            m_render_resources->display_font = m_render_resources->debug_font;
        } else {
            logger::info("Loading display font: {}", m_config.display_font());
            if (!m_render_resources->display_font.loadFromFile(m_config.display_font())) {
                logger::error("Failed to load display font from {}.", m_config.display_font());
                success = false;
            }
        }
    }

    if (m_real_time) {
        m_midi_input->for_each_track([this](Track& trk) { trk.set_max_events(config().max_events_per_track()); });
    }
    if (!success)
        return false;
    logger::info("Config file successfully reloaded from {}", m_config_file_path);
    return true;
}

void MIDIPlayer::generate_dust_texture()
{
    sf::RenderTexture target;
    target.create(config().particle_radius() * 256, config().particle_radius() * 256);
    target.setView(sf::View { {}, sf::Vector2f { target.getSize() } / 128.f });

    auto& shader = particle_shader();
    shader.setUniform("uRadius", config().particle_radius());
    shader.setUniform("uGlowSize", config().particle_glow_size());
    shader.setUniform("uCenter", sf::Vector2f {});
    sf::RectangleShape rs(sf::Vector2f(target.getSize()));
    rs.setOrigin(rs.getSize() / 2.f);
    target.draw(rs, &shader);
    target.display();

    // NOTE: SFML doesn't use move semantics, so we need to COPY the texture :(
    m_render_resources->dust_texture = target.getTexture();
    m_render_resources->dust_texture.setSmooth(true);
}

void MIDIPlayer::generate_minimap_texture()
{
    sf::RenderTexture target;
    target.create(1024, progress_bar_size({}).y);

    if (m_real_time) {
        return;
    }

    auto& input = *static_cast<MIDIFileInput*>(m_midi_input.get());
    sf::VertexArray varr(sf::Points);
    input.for_each_track([&](Track const& track) {
        for (auto const& event : track.events()) {
            if (auto note_event = dynamic_cast<NoteEvent*>(event.second.get())) {
                float position_x = (float)event.first / *input.end_tick() * target.getSize().x;
                float position_y = (note_event->key().to_piano_position() - view_offset_x) / view_size_x * target.getSize().y;
                varr.append(sf::Vertex({ position_x, position_y }, sf::Color { 255, 255, 255, 200 }));
            }
        }
    });
    target.draw(varr);

    target.display();

    // NOTE: SFML doesn't use move semantics, so we need to COPY the texture :(
    m_render_resources->minimap_texture = target.getTexture();
}

void MIDIPlayer::set_sound_playing(int index, int velocity, bool playing, sf::Color color)
{
    m_notes[index].is_played = playing;
    m_notes[index].color = color;
}

size_t MIDIPlayer::calculate_current_tick() const
{
    return m_midi_input->current_tick(*this);
}

bool MIDIPlayer::current_time_is(Config::Time time, size_t offset_in_frames) const
{
    switch (time.unit()) {
        case Config::Time::Unit::Ticks: {
            // FIXME: This should have its converter function
            auto offset_in_ticks = offset_in_frames * fps() / (microseconds_per_quarter_note() / (double)m_midi_input->ticks_per_quarter_note()) / 1000000.0;
            return current_tick() == time.value() + offset_in_ticks;
        }
        case Config::Time::Unit::Frames:
            return current_frame() == time.value() + offset_in_frames;
        case Config::Time::Unit::Seconds:
            return std::abs(current_frame() - (time.value() * fps() + offset_in_frames)) < 1.f / fps();
    }
    return false;
}

bool MIDIPlayer::is_in_interval_frame(Config::Time interval, size_t offset_in_frames) const
{
    switch (interval.unit()) {
        case Config::Time::Unit::Ticks:
            // FIXME: Implement this
            return false;
        case Config::Time::Unit::Frames:
            return (current_frame() - offset_in_frames) % (size_t)interval.value() == 0;
        case Config::Time::Unit::Seconds:
            return (current_frame() - offset_in_frames) % (size_t)(interval.value() * fps()) == 0;
    }
    return false;
}

size_t MIDIPlayer::frame_count_for_time(Config::Time time, size_t offset_in_frames) const
{
    switch (time.unit()) {
        case Config::Time::Unit::Ticks:
            // FIXME: This should have its converter function
            return time.value() * fps() / (microseconds_per_quarter_note() / (double)m_midi_input->ticks_per_quarter_note()) / 1000000.0 + offset_in_frames;
        case Config::Time::Unit::Frames:
            return time.value() + offset_in_frames;
        case Config::Time::Unit::Seconds:
            return time.value() * fps() + offset_in_frames;
    }
    return 0;
}

void MIDIPlayer::update()
{
    if (!is_paused()) {
        auto previous_current_tick = m_current_tick;
        m_midi_input->update(*this);
        m_current_tick = calculate_current_tick();

        if (m_config_file_watcher.file_was_modified())
            reload_config_file();

        m_config.update();
        auto events = m_midi_input->find_events_in_range(previous_current_tick, m_current_tick);

        m_events_executed += events.size();

        for (auto const& it : events) {
            it->execute(*this);
            if (m_midi_output) {
                m_events_written++;
                m_midi_output->write_event(*it);
            }
        }
    } else {
        m_current_tick = calculate_current_tick();
    }

    static std::default_random_engine engine;
    if (rand() % 20 == 0) {
        float rand_speed = std::uniform_real_distribution<float>(-2, 2)(engine);
        float rand_pos_x = std::uniform_real_distribution<float>(0, 128)(engine);
        float rand_pos_y = std::uniform_real_distribution<float>(-128, 0)(engine);
        int rand_time = std::uniform_int_distribution<int>(30, 45)(engine);
        m_winds.push_back({ 0, rand_speed, { rand_pos_x, rand_pos_y }, rand_time, rand_time });
    }
    for (auto& wind : m_winds) {
        double change_factor = wind.target_speed / (wind.start_time / 2.f);
        if (wind.time > wind.start_time / 2)
            wind.speed += change_factor;
        else
            wind.speed -= change_factor;
        wind.time--;
    }

    auto all_particles = { std::views::all(m_dust_particles), std::views::all(m_smoke_particles) };
    for (auto& particle : std::views::join(all_particles)) {
        particle.position += { particle.motion.x, particle.motion.y };
        particle.motion.x /= m_config.particle_x_drag();
        particle.motion.y -= particle.temperature * m_config.particle_temperature_multiplier();
        particle.motion.y += m_config.particle_gravity();
        for (auto const& wind : m_winds) {
            float dstx = particle.position.x - wind.pos.x;
            float dsty = particle.position.y - wind.pos.y;
            particle.motion.x -= std::min((double)m_config.particle_max_wind(),
                std::max((double)-m_config.particle_max_wind(), wind.speed / dsty / dsty));
        }
        particle.temperature *= m_config.particle_temperature_decay();
    }

    for (auto& label : m_labels)
        label.remaining_duration--;

    std::erase_if(m_dust_particles, [](auto const& particle) { return particle.temperature <= 1; });
    std::erase_if(m_smoke_particles, [](auto const& particle) { return particle.temperature <= 1; });
    std::erase_if(m_winds, [](auto const& wind) { return wind.time <= 0; });
    std::erase_if(m_labels, [](auto const& label) { return label.remaining_duration <= 0; });

    m_current_frame++;

    auto end_tick = m_midi_input->end_tick();
    if (end_tick.has_value() && current_tick() > end_tick.value())
        set_playing(false);
}

void MIDIPlayer::render_particles(sf::RenderTarget& target) const
{
    if (!m_smoke_particles.empty()) {
        // TODO: This probably can be further optimized
        sf::VertexArray varr(sf::Triangles, m_smoke_particles.size() * 6);
        size_t counter = 0;
        for (auto const& particle : m_smoke_particles) {
            auto color = particle.color;
            // TODO: Configurable alpha mul
            color.a = std::clamp<float>(particle.temperature / ParticleTemperatureMean * 255, 0.f, 255.f) * 0.05;

            float size = std::clamp<float>(1 - particle.temperature / ParticleTemperatureMean, 0.25, 1) * 4; // TODO: Configurable size
            float tex_size = m_render_resources->smoke_texture.getSize().x;

            varr[counter * 6 + 0] = sf::Vertex(
                { particle.position.x - size, particle.position.y - size },
                color, { 0, 0 });
            varr[counter * 6 + 1] = sf::Vertex(
                { particle.position.x - size, particle.position.y + size },
                color, { 0, tex_size });
            varr[counter * 6 + 2] = sf::Vertex(
                { particle.position.x + size, particle.position.y - size },
                color, { tex_size, 0 });
            varr[counter * 6 + 3] = sf::Vertex(
                { particle.position.x - size, particle.position.y + size },
                color, { 0, tex_size });
            varr[counter * 6 + 4] = sf::Vertex(
                { particle.position.x + size, particle.position.y + size },
                color, { tex_size, tex_size });
            varr[counter * 6 + 5] = sf::Vertex(
                { particle.position.x + size, particle.position.y - size },
                color, { tex_size, 0 });
            counter++;
        }
        sf::RenderStates states { &m_render_resources->smoke_texture };
        target.draw(varr, states);
    }
    if (!m_dust_particles.empty()) {
        // TODO: This probably can be further optimized
        sf::VertexArray varr(sf::Triangles, m_dust_particles.size() * 6);
        size_t counter = 0;
        for (auto const& particle : m_dust_particles) {
            auto color = particle.color;
            color.a = std::clamp<float>(particle.temperature / ParticleTemperatureMean * 255, 0.f, 255.f);

            float size = config().particle_radius();
            float tex_size = m_render_resources->dust_texture.getSize().x;

            varr[counter * 6 + 0] = sf::Vertex(
                { particle.position.x - size, particle.position.y - size },
                color, { 0, 0 });
            varr[counter * 6 + 1] = sf::Vertex(
                { particle.position.x - size, particle.position.y + size },
                color, { 0, tex_size });
            varr[counter * 6 + 2] = sf::Vertex(
                { particle.position.x + size, particle.position.y - size },
                color, { tex_size, 0 });
            varr[counter * 6 + 3] = sf::Vertex(
                { particle.position.x - size, particle.position.y + size },
                color, { 0, tex_size });
            varr[counter * 6 + 4] = sf::Vertex(
                { particle.position.x + size, particle.position.y + size },
                color, { tex_size, tex_size });
            varr[counter * 6 + 5] = sf::Vertex(
                { particle.position.x + size, particle.position.y - size },
                color, { tex_size, 0 });
            counter++;
        }
        sf::RenderStates states { &m_render_resources->dust_texture };
        states.blendMode = {
            sf::BlendMode::SrcAlpha, sf::BlendMode::DstAlpha, sf::BlendMode::Add,
            sf::BlendMode::One, sf::BlendMode::DstAlpha, sf::BlendMode::Add
        };
        target.draw(varr, states);
    }
}

void MIDIPlayer::render_overlay(sf::RenderTarget& target) const
{
    // Screen view things
    {
        auto target_size = target.getSize();
        sf::View old_view = target.getView();
        target.setView(sf::View { sf::FloatRect(0, 0, target_size.x, target_size.y) });

        // Gradient / Overlay
        sf::RectangleShape rs { sf::Vector2f { target_size } };
        m_render_resources->gradient_shader.setUniform("uColor", sf::Glsl::Vec4 { config().overlay_color() });
        target.draw(rs, sf::RenderStates { &m_render_resources->gradient_shader });

        // Labels
        for (auto const& label : m_labels) {
            sf::Text text(label.text, m_render_resources->display_font, config().label_font_size());
            auto bounds = text.getLocalBounds();
            float padding_left_right = config().label_font_size() * 100.f / 45.f;
            float padding_top_bottom = config().label_font_size() * 40.f / 45.f;
            RoundedEdgeRectangleShape rs { { bounds.width + padding_left_right, bounds.height + padding_top_bottom }, 10.f };
            rs.setPosition({ target_size.x / 2.f, target_size.y / 2.f + config().label_font_size() / 4.6f });
            rs.setOrigin(rs.getSize() / 2.f);
            auto calculate_alpha = [&](uint8_t max_alpha) -> uint8_t {
                if (label.remaining_duration < config().label_fade_time())
                    return label.remaining_duration * max_alpha / config().label_fade_time();
                if (label.remaining_duration > label.total_duration - config().label_fade_time())
                    return (label.total_duration - label.remaining_duration) * max_alpha / config().label_fade_time();
                return max_alpha;
            };
            auto bgcolor = config().background_color();
            bgcolor = config().background_color() + sf::Color(50, 50, 50);
            bgcolor.a = calculate_alpha(100);
            rs.setFillColor(bgcolor);
            target.draw(rs);
            text.setPosition(sf::Vector2f(target_size) / 2.f);
            text.setOrigin({ bounds.width / 2.f, bounds.height / 2.f });
            text.setFillColor(sf::Color(255, 255, 255, calculate_alpha(255)));
            target.draw(text);
        }

        target.setView(old_view);
    }

    // Light (background layer)
    auto render_light_with_blend_mode = [&](MIDIKey key, sf::BlendMode mode) {
        sf::Vector2f size { key.is_black() ? 0.7f : 1.f, 0.5f };
        sf::Vector2f extent { 5.f, 5.f };
        size += extent;
        sf::RectangleShape rs { size };
        rs.setPosition(key.to_piano_position() - (key.is_black() ? 0.15f : 0.f), -0.4f);
        rs.move(-extent / 2.f);
        rs.setFillColor(sf::Color::White);
        m_render_resources->notelight_shader.setUniform("uSize", size);
        m_render_resources->notelight_shader.setUniform("uColor", sf::Glsl::Vec4(m_notes[key].color));
        m_render_resources->notelight_shader.setUniform("uCenter", rs.getPosition() + size / 2.f);
        sf::RenderStates states(&m_render_resources->notelight_shader);
        states.blendMode = mode;
        target.draw(rs, states);
    };

    for (size_t s = 21; s <= 108; s++) {
        MIDIKey key { static_cast<uint8_t>(s) };
        if (m_notes[key].is_played) {
            render_light_with_blend_mode(key, sf::BlendAdd);
        }
    }

    // Piano
    auto upper_y_to_view_pos = target.mapPixelToCoords({ 0, static_cast<int>(target.getSize().y - piano_size_px) }).y;
    auto lower_y_to_view_pos = target.mapPixelToCoords({ 0, static_cast<int>(target.getSize().y) }).y;
    // a0 -- c8
    for (size_t s = 21; s <= 108; s++) {
        MIDIKey key { static_cast<uint8_t>(s) };
        if (!key.is_black()) {
            sf::RectangleShape rs { { 1.f, (lower_y_to_view_pos - upper_y_to_view_pos) } };
            rs.setPosition(key.to_piano_position(), 0.f);
            rs.setFillColor(m_notes[key].is_played ? m_notes[key].color : sf::Color(230, 230, 230));
            rs.setOutlineColor(sf::Color(150, 150, 150));
            rs.setOutlineThickness(0.1f);
            target.draw(rs);
        }
    }
    for (size_t s = 21; s <= 108; s++) {
        MIDIKey key { static_cast<uint8_t>(s) };
        if (key.is_black()) {
            sf::RectangleShape rs { { 0.7f, (lower_y_to_view_pos - upper_y_to_view_pos) * 3 / 5.f } };
            rs.setPosition(key.to_piano_position() - 0.15f, -0.1f);
            rs.setFillColor(m_notes[key].is_played ? m_notes[key].color * sf::Color(200, 200, 200) : sf::Color(50, 50, 50));
            target.draw(rs);
        }
    }

    // Light (on piano layer)
    for (size_t s = 21; s <= 108; s++) {
        MIDIKey key { static_cast<uint8_t>(s) };
        if (m_notes[key].is_played) {
            render_light_with_blend_mode(key,
                sf::BlendMode(sf::BlendMode::DstColor, sf::BlendMode::One, sf::BlendMode::Add,
                    sf::BlendMode::One, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add));
        }
    }
}

void MIDIPlayer::render_debug_info(sf::RenderTarget& target, DebugInfo const& debug_info) const
{
    sf::Vector2f target_size { target.getSize() };
    target.setView(sf::View({ 0, 0, target_size.x, target_size.y }));

    std::ostringstream oss;
    oss << get_stats_string(true);
    oss << "\n\n";
    oss << std::to_string(1.f / debug_info.last_fps_time.asSeconds()) + " fps\n";
    oss << "Particles: dust=" << m_dust_particles.size() << " smoke=" << m_smoke_particles.size() << std::endl;
    oss << "StaticTileColors: " << m_static_tile_colors.size() << std::endl;
    m_config.dump_stats(oss);

    if (!m_winds.empty()) {
        oss << "WIND:\n";
        for (auto const& wind : m_winds)
            oss << "    [" << wind.pos.x << "," << wind.pos.y << "] " << wind.speed << " " << wind.time << "\n";
    }

    sf::Text text { oss.str(), m_render_resources->debug_font, 10 };
    text.setPosition(5, 5);
    target.draw(text);
}

static std::string pretty_time(uint64_t seconds)
{
    std::ostringstream oss;
    oss << std::setfill('0');
    if (seconds > 3600)
        oss << std::setw(2) << (int)seconds / 3600 << ":" << std::setw(2); // The last setw is for minutes
    oss << ((int)seconds / 60) % 60 << ":"
        << std::setw(2) << (int)seconds % 60;
    return oss.str();
};

void MIDIPlayer::render_progress_bar(sf::RenderTarget& target) const
{
    sf::Vector2f target_size { target.getSize() };
    target.setView(sf::View({ 0, 0, target_size.x, target_size.y }));

    auto end_tick = m_midi_input->end_tick();
    if (end_tick) {
        auto size = progress_bar_size(sf::Vector2f(target.getSize()));
        float current_time = (double)current_tick() / m_midi_input->ticks_per_second(*this);
        float total_time = m_midi_input->end_tick().value() / m_midi_input->ticks_per_second(*this);

        RoundedEdgeRectangleShape rect { size, size.y / 2 };
        rect.setPosition({ target_size.x / 2.f - size.x / 2, 25 - size.y / 2 });
        rect.setFillColor(sf::Color { 100, 100, 100, 150 });
        target.draw(rect);

        rect.setFillColor(sf::Color::White);
        rect.setTexture(&m_render_resources->minimap_texture);
        target.draw(rect);
        rect.setTexture(nullptr);

        rect.setFillColor(sf::Color { 0, 160, 0, 150 });
        {
            auto old_view = target.getView();
            auto new_view = sf::View { { { rect.getPosition().x, 0 }, { size.x * current_time / total_time, target_size.y } } };
            new_view.setViewport({ 1 / 3.f, 0, current_time / total_time / 3.f, 1 });
            target.setView(new_view);
            target.draw(rect);
            target.setView(old_view);
        }

        sf::Text text_left { pretty_time(current_time), m_render_resources->display_font, 14 };
        text_left.setPosition(std::floor(target_size.x / 2.f - size.x / 2.f - text_left.getLocalBounds().width - 10 - text_left.getLocalBounds().left),
            std::floor(25 - text_left.getLocalBounds().height / 2.f - text_left.getLocalBounds().top));
        target.draw(text_left);

        sf::Text text_right { pretty_time(total_time), m_render_resources->display_font, 14 };
        text_right.setPosition(std::floor(target_size.x / 2.f + size.x / 2.f + 10),
            std::floor(25 - text_right.getLocalBounds().height / 2.f - text_left.getLocalBounds().top));
        target.draw(text_right);
    } else {
        sf::Text text { pretty_time((double)current_tick() / m_midi_input->ticks_per_second(*this)),
            m_render_resources->display_font, 14 };
        text.setPosition(std::floor(target_size.x / 2.f - text.getLocalBounds().width / 2.f), 10);
        target.draw(text);

        if (m_real_time && m_midi_output) {
            sf::CircleShape recording_circle { 6 };
            recording_circle.setPosition(10, 10);
            recording_circle.setFillColor(sf::Color::Red);
            target.draw(recording_circle);
        }
    }
}

void MIDIPlayer::render_pedals(sf::RenderTarget& target) const
{
    auto const PEDAL_TEXTURE_SIZE = m_render_resources->pedals_texture.getSize();
    sf::Vector2i const POSITION { static_cast<int>(target.getSize().x - PEDAL_TEXTURE_SIZE.x - 20), 10 };

    int const SIDE_PEDAL_WIDTH = PEDAL_TEXTURE_SIZE.x * 45 / 128;
    int const MIDDLE_PEDAL_WIDTH = PEDAL_TEXTURE_SIZE.x * 38 / 128;
    int const PEDAL_HEIGHT = PEDAL_TEXTURE_SIZE.y / 2;

    sf::IntRect SOFT_OFF_RECT { 0, 0, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };
    sf::IntRect SOFT_ON_RECT { 0, PEDAL_HEIGHT, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };

    sf::IntRect SOSTENUTO_OFF_RECT { SIDE_PEDAL_WIDTH, 0, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };
    sf::IntRect SOSTENUTO_ON_RECT { SIDE_PEDAL_WIDTH, PEDAL_HEIGHT, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };

    sf::IntRect SUSTAIN_OFF_RECT { SIDE_PEDAL_WIDTH + MIDDLE_PEDAL_WIDTH, 0, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };
    sf::IntRect SUSTAIN_ON_RECT { SIDE_PEDAL_WIDTH + MIDDLE_PEDAL_WIDTH, PEDAL_HEIGHT, SIDE_PEDAL_WIDTH, PEDAL_HEIGHT };

    sf::Sprite sprite(m_render_resources->pedals_texture);

    sprite.setPosition(sf::Vector2f(POSITION));
    sprite.setTextureRect(m_pedals.soft() ? SOFT_ON_RECT : SOFT_OFF_RECT);
    target.draw(sprite);

    sprite.setPosition(sf::Vector2f(POSITION) + sf::Vector2f(SIDE_PEDAL_WIDTH, 0));
    sprite.setTextureRect(m_pedals.sostenuto() ? SOSTENUTO_ON_RECT : SOSTENUTO_OFF_RECT);
    target.draw(sprite);

    sprite.setPosition(sf::Vector2f(POSITION) + sf::Vector2f(SIDE_PEDAL_WIDTH + MIDDLE_PEDAL_WIDTH, 0));
    sprite.setTextureRect(m_pedals.sustain() ? SUSTAIN_ON_RECT : SUSTAIN_OFF_RECT);
    target.draw(sprite);
}

std::string MIDIPlayer::get_stats_string(bool) const
{
    auto tick = current_tick();
    auto end_tick = m_midi_input->end_tick();

    std::ostringstream oss;
    auto elapsed_seconds = (double)tick / m_midi_input->ticks_per_second(*this);
    oss << pretty_time(elapsed_seconds);
    oss << " (Tick=" << tick << " Frame=" << current_frame() << " Second=" << std::fixed << std::setprecision(2) << elapsed_seconds << ")";

    if (!m_real_time && end_tick.has_value()) {
        oss << " / ";
        oss << pretty_time(end_tick.value() / m_midi_input->ticks_per_second(*this));
        oss << " (Ticks=" << end_tick.value() << ")";
        oss << " (" << 100 * tick / end_tick.value() << "%)";
    }

    oss << "  " << m_events_read << "R " << m_events_written << "W " << m_events_executed << "X";
    return oss.str();
}

void MIDIPlayer::render_background(sf::RenderTarget& target) const
{
    target.draw(m_config.background_image());
}

void MIDIPlayer::spawn_random_particles(sf::RenderTarget& target, MIDIKey key, sf::Color color, int velocity)
{
    auto size = target.getView().getSize();
    static std::default_random_engine engine;

    auto spawn_particle_of_type = [&](Particle::Type type) {
        float velocity_factor = (velocity - 64) / 2500.f + 0.03f;
        float rand_x_speed = (std::binomial_distribution<int>(100, 0.5)(engine) - 50) / 250.0;
        float rand_y_speed = -std::binomial_distribution<int>(100, 0.1)(engine) / 500.0 - velocity_factor;
        float offset = std::uniform_real_distribution<float>(-0.2, 0.2)(engine);
        float temperature = std::gamma_distribution<double>(ParticleTemperatureMean, 0.9)(engine);
        spawn_particle(type, Particle {
                                 .position = { key.to_piano_position() * size.x / MIDIPlayer::view_size_x + (key.is_black() ? 0.25f : 0.5f) + offset, 0 },
                                 .motion = { rand_x_speed, rand_y_speed },
                                 .color = sf::Color(std::min(255, color.r + 50), std::min(255, color.g + 50), std::min(255, color.b + 50)),
                                 .temperature = temperature,
                             });
    };
    for (size_t s = 0; s < m_config.particle_count(); s++) {
        spawn_particle_of_type(Particle::Type::Dust);
    }
    spawn_particle_of_type(Particle::Type::Smoke);
};

void MIDIPlayer::spawn_particle(Particle::Type type, Particle&& part)
{
    switch (type) {
        case Particle::Type::Dust:
            m_dust_particles.push_back(std::move(part));
            break;
        case Particle::Type::Smoke:
            m_smoke_particles.push_back(std::move(part));
            break;
    }
}

void MIDIPlayer::display_label(LabelType label, std::string text, int duration)
{
    m_labels.push_back({ label, std::move(text), duration, duration });
}

void MIDIPlayer::render(sf::RenderTarget& target, DebugInfo const& debug_info)
{
    target.clear(config().background_color());
    render_background(target);

    float aspect = static_cast<float>(target.getSize().x) / target.getSize().y;
    const float piano_size = MIDIPlayer::piano_size_px * (MIDIPlayer::view_size_x / aspect) / target.getSize().y;
    auto view = sf::View { sf::FloatRect(MIDIPlayer::view_offset_x, -MIDIPlayer::view_size_x / aspect + piano_size, MIDIPlayer::view_size_x, MIDIPlayer::view_size_x / aspect) };
    target.setView(view);
    if (m_real_time) {
        for (auto& it : m_ended_notes)
            it.reset();
        m_midi_input->for_first_events_starting_from_backwards(current_tick(), 4096, [&](Event& event) { event.render(*this, target); });
    } else {
        for (auto& it : m_started_notes)
            it.reset();
        m_midi_input->for_first_events_starting_from(current_tick() < 4096 ? 0 : current_tick() - 4096, 8192, [&](Event& event) { event.render(*this, target); });
    }
    render_particles(target);
    render_overlay(target);
    if (debug_info.full_info) {
        render_debug_info(target, debug_info);
    } else {
        render_progress_bar(target);
    }
    render_pedals(target);
}

void MIDIPlayer::print_config_help() const
{
    config().display_help();
}

sf::Texture* MIDIPlayer::get_background_image(std::string const& filename)
{
    if (!filename.empty()) {
        auto maybe_existing_texture = m_render_resources->background_textures.find(filename);
        if (maybe_existing_texture != m_render_resources->background_textures.end())
            return &maybe_existing_texture->second;

        auto new_texture = m_render_resources->background_textures.emplace(std::make_pair(filename, sf::Texture()));
        assert(new_texture.second);
        if (!new_texture.first->second.loadFromFile(filename)) {
            logger::error("Failed to load background image from {}.", filename);
            return nullptr;
        }
        return &new_texture.first->second;
    }
    return nullptr;
}
