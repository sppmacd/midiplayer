#include "MIDIPlayer.h"

#include "MIDIFile.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
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

MIDIPlayer::MIDIPlayer(MIDIInput& midi, RealTime real_time)
: m_midi(midi), m_real_time(real_time == RealTime::Yes), m_start_time { std::chrono::system_clock::now() }
{
    ensure_sounds_generated();
    if(
        m_gradient_shader.loadFromFile("res/shaders/gradient.vert", "res/shaders/gradient.frag")
        && m_note_shader.loadFromFile("res/shaders/note.vert", "res/shaders/note.frag")
        && m_particle_shader.loadFromFile("res/shaders/particle.vert", "res/shaders/particle.frag"))
        std::cerr << "Shaders loaded" << std::endl;

    if(m_font.loadFromFile("res/font.ttf"))
        std::cerr << "Font loaded" << std::endl;

    std::ifstream config_file("config.cfg");
    if(config_file.fail())
    {
        std::cerr << "WARNING: config.cfg doesn't exist." << std::endl;
        return;
    }

    while(true)
    {
        std::string command;
        if(!(config_file >> command))
            break;

        if(command == "color"sv)
        {
            std::vector<std::unique_ptr<Selector>> selectors;
            while(auto selector = Selector::read(config_file))
            {
                if(!selector && selectors.empty())
                {
                    std::cerr << "ERROR: invalid selector for 'color'" << std::endl;
                    exit(1);
                }
                selectors.push_back(std::move(selector));
            }

            int r, g, b, a = 128;
            if(!(config_file >> r >> g >> b))
            {
                std::cerr << "ERROR: color requires arguments: <selector> <r> <g> <b> [a]" << std::endl;
                exit(1);
            }
            if(!(config_file >> a))
            {
                a = 255;
                config_file.clear();
            }
            m_channel_colors.push_back(std::make_pair(std::move(selectors),
                sf::Color { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a) }));
        }
        else if(command == "default_color"sv)
        {
            int r, g, b, a = 128;
            if(!(config_file >> r >> g >> b))
            {
                std::cerr << "ERROR: default_color requires arguments: <r> <g> <b> [a]" << std::endl;
                exit(1);
            }
            if(!(config_file >> a))
            {
                a = 255;
                config_file.clear();
            }
            m_default_color = { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a) };
        }
        else if(command == "background_color"sv)
        {
            int r, g, b;
            if(!(config_file >> r >> g >> b))
            {
                std::cerr << "ERROR: background_color requires arguments: <r> <g> <b>" << std::endl;
                exit(1);
            }
            m_background_color = { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b) };
        }
        else if(command == "overlay_color"sv)
        {
            int r, g, b, a = 128;
            if(!(config_file >> r >> g >> b))
            {
                std::cerr << "ERROR: overlay_color requires arguments: <r> <g> <b> [a]" << std::endl;
                exit(1);
            }
            if(!(config_file >> a))
            {
                a = 255;
                config_file.clear();
            }
            m_overlay_color = { static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a) };
        }
        else if(command == "particle_count"sv)
        {
            int c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: particle_count requires arguments: <count>" << std::endl;
                exit(1);
            }
            m_particle_count = c;
        }
        else if(command == "particle_radius"sv)
        {
            float c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: particle_radius requires arguments: <radius>" << std::endl;
                exit(1);
            }
            m_particle_radius = c;
        }
        else if(command == "particle_glow_size"sv)
        {
            float c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: particle_glow_size requires arguments: <radius>" << std::endl;
                exit(1);
            }
            m_particle_glow_size = c;
        }
        else if(command == "max_events_per_track"sv)
        {
            size_t c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: max_events_per_track requires arguments: <count>" << std::endl;
                exit(1);
            }
            m_max_events_per_track = c;
        }
        else if(command == "real_time_scale"sv)
        {
            double c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: real_time_scale requires arguments: <factor>" << std::endl;
                exit(1);
            }
            m_real_time_scale = c;
        }
        else if(command == "play_scale"sv)
        {
            double c;
            if(!(config_file >> c))
            {
                std::cerr << "ERROR: play_scale requires arguments: <factor>" << std::endl;
                exit(1);
            }
            m_play_scale = c;
        }
        else if(command == "background_image"sv)
        {
            config_file >> std::ws;
            std::string path;
            if(!(std::getline(config_file, path)))
            {
                std::cerr << "ERROR: background_image requires arguments: <path...>" << std::endl;
                exit(1);
            }
            if(!m_background_texture.loadFromFile(path))
            {
                std::cerr << "WARNING: Failed to load background image" << std::endl;
                continue;
            }
            m_background_sprite.setTexture(m_background_texture);
        }
        else
        {
            // TODO: Help
            std::cerr << "ERROR: Invalid config command: " << command << std::endl;
            exit(1);
        }
    }

    if(m_real_time)
    {
        m_midi.for_each_track([this](Track& trk)
            { trk.set_max_events(m_max_events_per_track); });
    }

    generate_particle_texture();
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
    target.create(m_particle_radius * 256, m_particle_radius * 256);
    target.setView(sf::View { {}, sf::Vector2f { target.getSize() } / 128.f });

    auto& shader = particle_shader();
    shader.setUniform("uRadius", m_particle_radius);
    shader.setUniform("uGlowSize", m_particle_glow_size);
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
    return m_midi.current_tick(*this);
}

void MIDIPlayer::update()
{
    auto previous_current_tick = m_current_tick;
    m_midi.update(*this);
    m_current_tick = current_tick();
    auto events = m_midi.find_events_in_range(previous_current_tick, m_current_tick);

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

    std::erase_if(m_particles, [](auto const& particle)
        { return particle.lifetime <= 0; });
    std::erase_if(m_winds, [](auto const& wind)
        { return wind.time <= 0; });
}

sf::Color MIDIPlayer::resolve_color(NoteEvent& event) const
{
    for(auto const& pair : m_channel_colors)
    {
        bool matched = true;
        for(auto const& selector : pair.first)
        {
            if(!selector->matches(*this, event))
            {
                matched = false;
                break;
            }
        }
        if(matched)
            return pair.second;
    }
    return m_default_color;
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
        color.r = std::min(255, (int)color.r + 20);
        color.g = std::min(255, (int)color.g + 20);
        color.b = std::min(255, (int)color.b + 20);
        color.a = particle.lifetime * 255 / particle.start_lifetime;

        float size = m_particle_radius;
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
    // Gradient / Overlay
    {
        sf::View old_view = target.getView();
        target.setView(sf::View { sf::FloatRect(0, 0, target.getSize().x, target.getSize().y) });
        sf::RectangleShape rs { sf::Vector2f { target.getSize() } };
        m_gradient_shader.setUniform("uColor", sf::Glsl::Vec4 { m_overlay_color });
        target.draw(rs, sf::RenderStates { &m_gradient_shader });
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

void MIDIPlayer::render_debug_info(sf::RenderTarget& target, Preview preview, sf::Time last_fps_time) const
{
    sf::Vector2f target_size { target.getSize() };
    target.setView(sf::View({ 0, 0, target_size.x, target_size.y }));
    auto tick = current_tick();
    auto end_tick = m_midi.end_tick();
    std::ostringstream oss;
    oss << (preview == Preview::Yes ? "Tick=" : "") << tick;
    if(!m_real_time && end_tick != 0)
        oss << "/" << end_tick << " (" << 100 * tick / end_tick << "%)";
    if(preview == Preview::Yes)
    {
        oss << "\n\n";
        oss << std::to_string(1.f / last_fps_time.asSeconds()) + " fps\n";
        if(!m_winds.empty())
        {
            oss << "WIND:\n";
            for(auto const& wind : m_winds)
                oss << "    [" << wind.pos.x << "," << wind.pos.y << "] " << wind.speed << " " << wind.time << "\n";
        }
        oss << m_particles.size() << " particles" << std::endl;
    }

    sf::Text text { oss.str(), m_font, 10 };
    text.setPosition(5, 5);
    target.draw(text);
}

void MIDIPlayer::render_background(sf::RenderTarget& target) const
{
    auto* texture = m_background_sprite.getTexture();
    if(!texture)
        return;
    target.setView(sf::View { sf::FloatRect { {}, sf::Vector2f { texture->getSize() } } });
    target.draw(m_background_sprite);
}

void MIDIPlayer::render(sf::RenderTarget& target, Preview preview, sf::Time last_fps_time)
{
    target.clear(m_background_color);
    render_background(target);

    float aspect = static_cast<float>(target.getSize().x) / target.getSize().y;
    const float piano_size = MIDIPlayer::piano_size_px * (MIDIPlayer::view_size_x / aspect) / target.getSize().y;
    auto view = sf::View { sf::FloatRect(MIDIPlayer::view_offset_x, -MIDIPlayer::view_size_x / aspect + piano_size, MIDIPlayer::view_size_x, MIDIPlayer::view_size_x / aspect) };
    target.setView(view);
    if(m_real_time)
    {
        ended_notes().clear();
        m_midi.for_each_event_backwards([&](Event& event)
            { event.render(*this, target); });
    }
    else
    {
        started_notes().clear();
        m_midi.for_each_event([&](Event& event)
            { event.render(*this, target); });
    }
    render_particles(target);
    render_overlay(target);
    render_debug_info(target, preview, last_fps_time);
}
