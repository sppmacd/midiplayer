#include "MIDIPlayer.h"

#include "MIDIFile.h"

#include <SFML/Audio.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <cassert>
#include <climits>
#include <cmath>
#include <fstream>
#include <random>

using namespace std::literals;

// https://github.com/MajicDesigns/MD_MusicTable/blob/master/src/MD_MusicTable_Data.cpp
static float const s_frequency_lookup_table[] {
    261.63,  // C4 Middle C
    277.18,  // C#4
    293.66,  // D4
    311.13,  // D#4
    329.63,  // E4
    349.23,  // F4
    369.99,  // F#4
    392.00,  // G4
    415.30,  // G#4
    440.00,  // A4 440Hz Standard Tuning
    466.16,  // A#4
    493.88,  // B4
};

struct Note
{
    sf::SoundBuffer buffer;
    sf::Sound sound;
} s_notes[128];

static int proper_modulo(int a, int b)
{
    int result = a < 0 ? ((a + 1) % b + b - 1) : a % b;
    if(!(result < b))
    {
        std::cout << "proper_mod " << a << "%" << b << "=" << result << std::endl;
        assert(false);
    }
    return result;
}

static int proper_division(int a, int b)
{
    return a < 0 ? a / b - 1 : a / b;
}

static float index_to_frequency(int index)
{
    int c4_relative_index = index - 60;
    return s_frequency_lookup_table[proper_modulo(c4_relative_index, 12)] * std::pow(2, proper_division(c4_relative_index, 12));
}

void generate_sound(sf::SoundBuffer& buf, size_t sample_count)
{
    std::vector<int16_t> samples;
    samples.resize(sample_count);

    for(size_t s = 0; s < sample_count; s++)
        samples[s] = std::sin(static_cast<double>(s) * 6.28 / sample_count) * 32767;

    if(!buf.loadFromSamples(samples.data(), sample_count, 1, 48000))
        std::cout << "loadFromSamples failed: " << sample_count << std::endl;
}

MIDIPlayer::MIDIPlayer(MIDI& midi, RealTime real_time)
: m_midi(midi), m_real_time(real_time)
{
    ensure_sounds_generated();
    if(
           m_note_shader.loadFromFile("res/shaders/note.vert", "res/shaders/note.frag")
        && m_particle_shader.loadFromFile("res/shaders/particle.vert", "res/shaders/particle.frag")
    )
        std::cout << "Shaders loaded" << std::endl;

    std::ifstream config_file("config.cfg");
    if(config_file.fail())
    {
        std::cout << "WARNING: config.cfg doesn't exist." << std::endl;
        return;
    }
    while(true)
    {
        std::string command;
        if(!(config_file >> command))
            break;

        if(command == "channel_color"sv)
        {
            int channel;
            int r, g, b, a = 128;
            if(!(config_file >> channel >> r >> g >> b))
            {
                std::cout << "ERROR: channel_color requires arguments: <channel> <r> <g> <b> [a]" << std::endl;
                exit(1);
            }
            if(!(config_file >> a))
            {
                config_file.clear();
                a = 128;
            }
            auto color = sf::Color{static_cast<uint8_t>(r),static_cast<uint8_t>(g),static_cast<uint8_t>(b),static_cast<uint8_t>(a)};
            m_channel_colors[channel] = color;
        }
        else if(command == "fallback_channel_color"sv)
        {
            int r, g, b, a = 128;
            if(!(config_file >> r >> g >> b))
            {
                std::cout << "ERROR: fallback_channel_color requires arguments: <r> <g> <b> [a]" << std::endl;
                exit(1);
            }
            if(!(config_file >> a))
            {
                config_file.clear();
                a = 128;
            }
            m_fallback_channel_color = {static_cast<uint8_t>(r),static_cast<uint8_t>(g),static_cast<uint8_t>(b),static_cast<uint8_t>(a)};
        }
        else if(command == "particle_count"sv)
        {
            int c;
            if(!(config_file >> c))
            {
                std::cout << "ERROR: particle_count requires arguments: <count>" << std::endl;
                exit(1);
            }
            m_particle_count = c;
        }
        else if(command == "particle_radius"sv)
        {
            float c;
            if(!(config_file >> c))
            {
                std::cout << "ERROR: particle_radius requires arguments: <radius>" << std::endl;
                exit(1);
            }
            m_particle_radius = c;
        }
        else if(command == "particle_glow_size"sv)
        {
            float c;
            if(!(config_file >> c))
            {
                std::cout << "ERROR: particle_glow_size requires arguments: <radius>" << std::endl;
                exit(1);
            }
            m_particle_glow_size = c;
        }
        else
        {
            // TODO: Help
            std::cout << "ERROR: Invalid config command: " << command << std::endl;
            exit(1);
        }
    }
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

void MIDIPlayer::set_sound_playing(int index, int velocity, bool playing)
{
    s_notes[index].sound.setVolume((float)velocity / 1.27);
    if(playing)
        s_notes[index].sound.play();
    else
        s_notes[index].sound.stop();
}

size_t MIDIPlayer::ticks_per_frame() const
{
    return (static_cast<double>(m_midi.ticks_per_quarter_note()) / m_microseconds_per_quarter_note) / (m_fps / 1000000.0);
}

void MIDIPlayer::update()
{
    m_midi.update();
    auto ticks = ticks_per_frame();
    auto events = m_midi.find_events_in_range(m_current_tick, m_current_tick + ticks);

    for(auto const& it: events)
        it->execute(*this);

    static std::default_random_engine engine;
    if(rand() % 100 == 0)
    {
        float rand_speed = std::uniform_real_distribution<float>(-5, 5)(engine);
        float rand_pos_x = std::uniform_real_distribution<float>(0, 128)(engine);
        float rand_pos_y = std::uniform_real_distribution<float>(-128, 0)(engine);
        int rand_time = std::uniform_int_distribution<int>(30, 45)(engine);
        m_wind = {0, rand_speed, {rand_pos_x, rand_pos_y}, rand_time, rand_time};
    }
    if(m_wind.time-- == 0)
        m_wind.speed = 0;
    else
    {
        double change_factor = m_wind.speed / (m_wind.start_time / 2.f);
        if((m_wind.time > m_wind.start_time / 2) ^ (m_wind.target_speed > 0))
            m_wind.speed += change_factor;
        else
            m_wind.speed -= change_factor;
    }

    for(auto& particle: m_particles)
    {
        particle.position += particle.motion;
        particle.motion.x /= 1.05f;
        particle.motion.y /= 1.01f;
        float dstx = particle.position.x - m_wind.pos.x;
        float dsty = particle.position.y - m_wind.pos.y;
        particle.motion.x += std::min(0.005, std::max(-0.005, m_wind.speed / dsty / dsty));
        particle.motion.y += std::min(0.005, std::max(-0.005, m_wind.speed / dstx / dstx));
        particle.lifetime--;
    }

    std::erase_if(m_particles, [](auto const& particle) { return particle.lifetime <= 0; });

    m_current_tick += ticks;
}

void MIDIPlayer::render_particles(sf::RenderTarget& target) const
{
    sf::CircleShape cs(std::abs(m_wind.speed));
    cs.setOrigin(std::abs(m_wind.speed), std::abs(m_wind.speed));
    cs.setPosition(m_wind.pos);
    cs.setFillColor(m_wind.speed < 0 ? sf::Color::Red : sf::Color::Green);
    target.draw(cs);
    
    auto& shader = particle_shader();
    shader.setUniform("uRadius", m_particle_radius);
    shader.setUniform("uGlowSize", m_particle_glow_size);
    for(auto const& particle: m_particles)
    {
        auto color = particle.color;
        color.a = particle.lifetime * 255 / particle.start_lifetime;
        sf::CircleShape cs(m_particle_radius);
        cs.setFillColor(color);
        cs.setPosition(particle.position);
        cs.setOrigin(m_particle_radius, m_particle_radius);
        shader.setUniform("uCenter", particle.position);
        target.draw(cs, sf::RenderStates{&shader});
        //std::cout << center.x << ";" << center.y << std::endl;
    }
}
