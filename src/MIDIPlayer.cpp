#include "MIDIPlayer.h"

#include "Logger.h"
#include "MIDIFile.h"
#include "Resources.h"
#include "RoundedEdgeRectangleShape.hpp"

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <cassert>
#include <climits>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>

using namespace std::literals;

// https://github.com/MajicDesigns/MD_MusicTable/blob/master/src/MD_MusicTable_Data.cpp
static float const s_frequency_lookup_table[] {
    261.63, // C4 Middle C
    277.18, // C#4
    293.66, // D4
    311.13, // D#4
    329.63, // E4
    349.23, // F4
    369.99, // F#4
    392.00, // G4
    415.30, // G#4
    440.00, // A4 440Hz Standard Tuning
    466.16, // A#4
    493.88, // B4
};

struct Note
{
    sf::SoundBuffer buffer;
    sf::Sound sound;
    bool is_playing = false;
    sf::Color color;
} s_notes[128];

static float index_to_frequency(int index)
{
    return s_frequency_lookup_table[index % 12] * std::pow(2, index / 12 - 4);
}

void generate_sound(sf::SoundBuffer& buf, size_t sample_count)
{
    std::vector<int16_t> samples;
    samples.resize(sample_count);

    for(size_t s = 0; s < sample_count; s++)
        samples[s] = std::sin(static_cast<double>(s) * 6.28 / sample_count) * 32767;

    if(!buf.loadFromSamples(samples.data(), sample_count, 1, 48000))
        std::cerr << "loadFromSamples failed: " << sample_count << std::endl;
}

MIDIPlayer::MIDIPlayer()
{
}

void MIDIPlayer::initialize(RealTime real_time, std::unique_ptr<MIDIInput>&& input, std::unique_ptr<MIDIOutput>&& output)
{
    m_real_time = real_time == RealTime::Yes;
    m_start_time = std::chrono::system_clock::now();
    m_midi_output = std::move(output);
    m_midi_input = std::move(input);

    auto resource_path = find_resource_path();
    std::cerr << "Resource path: " << resource_path << std::endl;
    ensure_sounds_generated();
    if(
        m_gradient_shader.loadFromFile(resource_path + "/shaders/gradient.vert", resource_path + "/shaders/gradient.frag")
        && m_note_shader.loadFromFile(resource_path + "/shaders/note.vert", resource_path + "/shaders/note.frag")
        && m_particle_shader.loadFromFile(resource_path + "/shaders/particle.vert", resource_path + "/shaders/particle.frag"))
        std::cerr << "Shaders loaded" << std::endl;

    if(m_debug_font.loadFromFile(resource_path + "/dejavu-sans-mono.ttf"))
        std::cerr << "Font loaded" << std::endl;

    m_initialized = true;
}

bool MIDIPlayer::load_config_file(std::string const& path)
{
    m_config_file_path = path;
    m_config_file_watcher = FileWatcher(path);
    return reload_config_file();
}

bool MIDIPlayer::reload_config_file()
{
    // FIXME: This should be moved to a separate class which would be just default-assigned here.
    bool success = m_config.reload(m_config_file_path);
    generate_particle_texture();
    if(m_real_time)
    {
        m_midi_input->for_each_track([this](Track& trk)
            { trk.set_max_events(config().max_events_per_track()); });
    }
    if(!success)
        return false;
    logger::info("Config file successfully reloaded from {}", m_config_file_path);
    return true;
}

void MIDIPlayer::ensure_sounds_generated()
{
    static bool s_generated = false;
    if(s_generated)
        return;
    s_generated = true;

    for(int i = 0; i < 128; i++)
    {
        float fq = index_to_frequency(i);
        generate_sound(s_notes[i].buffer, 48000 / fq);
        s_notes[i].sound.setBuffer(s_notes[i].buffer);
        s_notes[i].sound.setLoop(true);
    }
}
void MIDIPlayer::generate_particle_texture()
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
    m_particle_texture = target.getTexture();
    m_particle_texture.setSmooth(true);
}

void MIDIPlayer::set_sound_playing(int index, int velocity, bool playing, sf::Color color)
{
    s_notes[index].sound.setVolume((float)velocity / 1.27);
    s_notes[index].is_playing = playing;
    s_notes[index].color = color;
    if(playing)
        s_notes[index].sound.play();
    else
        s_notes[index].sound.stop();
}

size_t MIDIPlayer::current_tick() const
{
    return m_midi_input->current_tick(*this);
}

void MIDIPlayer::update()
{
    if(m_config_file_watcher.file_was_modified())
        reload_config_file();
    auto previous_current_tick = m_current_tick;
    m_midi_input->update(*this);
    m_current_tick = current_tick();
    auto events = m_midi_input->find_events_in_range(previous_current_tick, m_current_tick);

    for(auto const& it : events)
    {
        it->execute(*this);
        if(m_midi_output)
            m_midi_output->write_event(*it);
    }

    static std::default_random_engine engine;
    if(rand() % 20 == 0)
    {
        float rand_speed = std::uniform_real_distribution<float>(-2, 2)(engine);
        float rand_pos_x = std::uniform_real_distribution<float>(0, 128)(engine);
        float rand_pos_y = std::uniform_real_distribution<float>(-128, 0)(engine);
        int rand_time = std::uniform_int_distribution<int>(30, 45)(engine);
        m_winds.push_back({ 0, rand_speed, { rand_pos_x, rand_pos_y }, rand_time, rand_time });
    }
    for(auto& wind : m_winds)
    {
        double change_factor = wind.target_speed / (wind.start_time / 2.f);
        if(wind.time > wind.start_time / 2)
            wind.speed += change_factor;
        else
            wind.speed -= change_factor;
        wind.time--;
    }

    for(auto& particle : m_particles)
    {
        particle.position += { particle.motion.x, particle.motion.y };
        particle.motion.x /= 1.05f;
        particle.motion.y /= 1.01f;
        for(auto const& wind : m_winds)
        {
            float dstx = particle.position.x - wind.pos.x;
            float dsty = particle.position.y - wind.pos.y;
            particle.motion.x -= std::min(0.00225, std::max(-0.00225, wind.speed / dsty / dsty));
        }
        particle.lifetime--;
    }

    for(auto& label : m_labels)
        label.remaining_duration--;

    std::erase_if(m_particles, [](auto const& particle)
        { return particle.lifetime <= 0; });
    std::erase_if(m_winds, [](auto const& wind)
        { return wind.time <= 0; });
    std::erase_if(m_labels, [](auto const& label)
        { return label.remaining_duration <= 0; });

    m_current_frame++;
}

void MIDIPlayer::render_particles(sf::RenderTarget& target) const
{
    if(m_particles.empty())
        return;

    // TODO: This probably can be further optimized
    sf::VertexArray varr(sf::Triangles, m_particles.size() * 6);
    size_t counter = 0;
    for(auto const& particle : m_particles)
    {
        auto color = particle.color;
        color.a = particle.lifetime * 255 / particle.start_lifetime;

        float size = config().particle_radius();
        float tex_size = m_particle_texture.getSize().x;

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
    sf::RenderStates states { &m_particle_texture };
    states.blendMode = {
        sf::BlendMode::SrcAlpha, sf::BlendMode::DstAlpha, sf::BlendMode::Add,
        sf::BlendMode::One, sf::BlendMode::DstAlpha, sf::BlendMode::Add
    };
    target.draw(varr, states);
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
        m_gradient_shader.setUniform("uColor", sf::Glsl::Vec4 { config().overlay_color() });
        target.draw(rs, sf::RenderStates { &m_gradient_shader });

        // Labels
        for(auto const& label : m_labels)
        {
            sf::Text text(label.text, config().display_font(), config().label_font_size());
            auto bounds = text.getLocalBounds();
            float padding_left_right = config().label_font_size() * 100.f / 45.f;
            float padding_top_bottom = config().label_font_size() * 40.f / 45.f;
            RoundedEdgeRectangleShape rs { { bounds.width + padding_left_right, bounds.height + padding_top_bottom }, 10.f };
            rs.setPosition({ target_size.x / 2.f, target_size.y / 2.f + config().label_font_size() / 4.6f });
            rs.setOrigin(rs.getSize() / 2.f);
            auto calculate_alpha = [&](uint8_t max_alpha) -> uint8_t
            {
                if(label.remaining_duration < config().label_fade_time())
                    return label.remaining_duration * max_alpha / config().label_fade_time();
                if(label.remaining_duration > label.total_duration - config().label_fade_time())
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

    // Piano
    auto upper_y_to_view_pos = target.mapPixelToCoords({ 0, static_cast<int>(target.getSize().y - piano_size_px) }).y;
    auto lower_y_to_view_pos = target.mapPixelToCoords({ 0, static_cast<int>(target.getSize().y) }).y;
    // a0 -- c8
    for(size_t s = 21; s <= 108; s++)
    {
        MIDIKey key { static_cast<uint8_t>(s) };
        if(!key.is_black())
        {
            sf::RectangleShape rs { { 1.f, (lower_y_to_view_pos - upper_y_to_view_pos) } };
            rs.setPosition(key.to_piano_position(), 0.f);
            rs.setFillColor(s_notes[key].is_playing ? s_notes[key].color : sf::Color(230, 230, 230));
            rs.setOutlineColor(sf::Color(150, 150, 150));
            rs.setOutlineThickness(0.1f);
            target.draw(rs);
        }
    }
    for(size_t s = 21; s <= 108; s++)
    {
        MIDIKey key { static_cast<uint8_t>(s) };
        if(key.is_black())
        {
            sf::RectangleShape rs { { 0.7f, (lower_y_to_view_pos - upper_y_to_view_pos) * 3 / 5.f } };
            rs.setPosition(key.to_piano_position() - 0.15f, -0.1f);
            rs.setFillColor(s_notes[key].is_playing ? s_notes[key].color * sf::Color(200, 200, 200) : sf::Color(50, 50, 50));
            target.draw(rs);
        }
    }
}

void MIDIPlayer::render_debug_info(sf::RenderTarget& target, DebugInfo const& debug_info) const
{
    sf::Vector2f target_size { target.getSize() };
    target.setView(sf::View({ 0, 0, target_size.x, target_size.y }));
    auto tick = current_tick();
    auto end_tick = m_midi_input->end_tick();
    std::ostringstream oss;
    oss << (debug_info.full_info ? "Tick=" : "") << tick;
    if(!m_real_time && end_tick != 0)
        oss << "/" << end_tick << " (" << 100 * tick / end_tick << "%)";
    if(debug_info.full_info)
    {
        oss << "\n\n";
        oss << std::to_string(1.f / debug_info.last_fps_time.asSeconds()) + " fps\n";
        if(!m_winds.empty())
        {
            oss << "WIND:\n";
            for(auto const& wind : m_winds)
                oss << "    [" << wind.pos.x << "," << wind.pos.y << "] " << wind.speed << " " << wind.time << "\n";
        }
        oss << m_particles.size() << " particles" << std::endl;
    }

    sf::Text text { oss.str(), m_debug_font, 10 };
    text.setPosition(5, 5);
    target.draw(text);
}

void MIDIPlayer::render_background(sf::RenderTarget& target) const
{
    auto* texture = config().background_sprite().getTexture();
    if(!texture)
        return;
    target.setView(sf::View { sf::FloatRect { {}, sf::Vector2f { texture->getSize() } } });
    target.draw(config().background_sprite());
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
    if(m_real_time)
    {
        ended_notes().clear();
        m_midi_input->for_each_event_backwards([&](Event& event)
            { event.render(*this, target); });
    }
    else
    {
        started_notes().clear();
        m_midi_input->for_each_event([&](Event& event)
            { event.render(*this, target); });
    }
    render_particles(target);
    render_overlay(target);
    render_debug_info(target, debug_info);
}

void MIDIPlayer::print_config_help() const
{
    config().display_help();
}
