#pragma once

#include "Config/AnimatableProperty.h"
#include "Config/Configuration.h"
#include "Config/Property.h"
#include "Config/Reader.h"
#include "Config/Selector.h"
#include "FileWatcher.h"

#include <SFML/Graphics.hpp>
#include <list>
#include <memory>

inline sf::Color operator*(sf::Color const& left, double right)
{
    return { static_cast<sf::Uint8>(left.r * right),
        static_cast<sf::Uint8>(left.g * right),
        static_cast<sf::Uint8>(left.b * right),
        static_cast<sf::Uint8>(left.a * right) };
}

class MIDIPlayerConfig
{
public:
    MIDIPlayerConfig(MIDIPlayer& player);

    bool reload(std::string const& path);
    void display_help() const;
    void update();

    std::string display_font() const { return m_properties.display_font; }
    sf::Color default_color() const { return m_properties.default_color; }
    sf::Color background_color() const { return m_properties.background_color; }
    sf::Color overlay_color() const { return m_properties.overlay_color; }
    int particle_count() const { return m_properties.particle_count; }
    float particle_radius() const { return m_properties.particle_radius; }
    float particle_glow_size() const { return m_properties.particle_glow_size; }
    size_t max_events_per_track() const { return m_properties.max_events_per_track; }
    double real_time_scale() const { return m_properties.real_time_scale; }
    double play_scale() const { return m_properties.play_scale; }
    int label_font_size() const { return m_properties.label_font_size; }
    int label_fade_time() const { return m_properties.label_fade_time; }
    std::string background_image() const { return m_properties.background_image; }

    sf::Color resolve_color(MIDIPlayer const& player, NoteEvent& event) const;
    void set_property(std::string const& name, std::vector<Config::PropertyParameter> const& params);

private:
    Config::Info m_info;
    Config::Reader m_reader;

    struct Properties
    {
        std::string display_font;
        std::list<std::pair<Config::SelectorList, sf::Color>> channel_colors;
        sf::Color default_color { 255, 100, 100 };
        Config::AnimatableProperty<sf::Color> background_color { sf::Color(10, 10, 10) };
        sf::Color overlay_color { 5, 5, 5 };
        int particle_count = 2;
        float particle_radius = 0.5;
        float particle_glow_size = 0.1;
        size_t max_events_per_track = 4096;
        Config::AnimatableProperty<double> real_time_scale { 0.05 };
        Config::AnimatableProperty<double> play_scale { 0.05 };
        int label_font_size = 50;
        int label_fade_time = 30;
        std::string background_image;
    } m_properties;
};