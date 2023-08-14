#pragma once

#include "AnimatableBackground.h"
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

class MIDIPlayerConfig {
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
    float particle_x_drag() const { return m_properties.particle_x_drag; }
    float particle_temperature_multiplier() const { return m_properties.particle_temperature_multiplier; }
    float particle_gravity() const { return m_properties.particle_gravity; }
    float particle_max_wind() const { return m_properties.particle_max_wind; }
    float particle_temperature_decay() const { return m_properties.particle_temperature_decay; }
    float smoke_alpha_mul() const { return m_properties.m_smoke_alpha_mul; }
    float smoke_size_mul() const { return m_properties.m_smoke_size_mul; }
    size_t max_events_per_track() const { return m_properties.max_events_per_track; }
    double scale() const { return m_properties.scale; }
    int label_font_size() const { return m_properties.label_font_size; }
    int label_fade_time() const { return m_properties.label_fade_time; }
    BlendedBackground background_image() const { return m_properties.background_image; }

    void set_property(std::string const& name, std::vector<Config::PropertyParameter> const& params);

    void dump_stats(std::ostream&) const;

    auto const& info() const { return m_info; }

private:
    Config::Info m_info;
    Config::Reader m_reader;

    struct Properties {
        std::string display_font;
        sf::Color default_color { 0x36, 0x96, 0xC8 };
        Config::AnimatableProperty<sf::Color> background_color { sf::Color(4, 6, 8) };
        sf::Color overlay_color { 2, 3, 4 };
        int particle_count = 5;
        float particle_radius = 1;
        float particle_glow_size = 0.03;
        float particle_x_drag = 1.01;
        float particle_temperature_multiplier = 0.000024;
        float particle_gravity = 0.0008;
        float particle_max_wind = 0.00125;
        float particle_temperature_decay = 0.97;
        float m_smoke_alpha_mul = 0.01;
        float m_smoke_size_mul = 3;
        size_t max_events_per_track = 4096;
        Config::AnimatableProperty<double> scale { 0.02 };
        int label_font_size = 50;
        int label_fade_time = 30;
        Config::AnimatableProperty<AnimatableBackground> background_image;
    } m_properties;
};
