#pragma once

#include "ConfigFileReader.h"
#include "FileWatcher.h"
#include "Selector.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

struct Config
{
public:
    Config();

    bool reload(std::string const& path);
    void display_help() const { m_reader.display_help(); }

    sf::Font const& display_font() const { return m_options.display_font; }
    sf::Color default_color() const { return m_options.default_color; }
    sf::Color background_color() const { return m_options.background_color; }
    sf::Color overlay_color() const { return m_options.overlay_color; }
    int particle_count() const { return m_options.particle_count; }
    float particle_radius() const { return m_options.particle_radius; }
    float particle_glow_size() const { return m_options.particle_glow_size; }
    size_t max_events_per_track() const { return m_options.max_events_per_track; }
    double real_time_scale() const { return m_options.real_time_scale; }
    double play_scale() const { return m_options.play_scale; }
    int label_font_size() const { return m_options.label_font_size; }
    int label_fade_time() const { return m_options.label_fade_time; }
    sf::Sprite const& background_sprite() const { return m_options.background_sprite; }

    sf::Color resolve_color(MIDIPlayer const& player, NoteEvent& event) const;

private:
    ConfigFileReader m_reader;

    struct Options
    {
        sf::Font display_font;
        std::vector<std::pair<std::vector<std::unique_ptr<Selector>>, sf::Color>> channel_colors;
        sf::Color default_color { 100, 100, 255 };
        sf::Color background_color { 10, 10, 10 };
        sf::Color overlay_color { 5, 5, 5 };
        int particle_count = 2;
        float particle_radius = 0.5;
        float particle_glow_size = 0.1;
        size_t max_events_per_track = 4096;
        double real_time_scale = 0.05;
        double play_scale = 0.05;
        int label_font_size = 50;
        int label_fade_time = 30;
        sf::Texture background_texture;
        sf::Sprite background_sprite;
    } m_options;
};
