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
    return { static_cast<std::uint8_t>(left.r * right),
        static_cast<std::uint8_t>(left.g * right),
        static_cast<std::uint8_t>(left.b * right),
        static_cast<std::uint8_t>(left.a * right) };
}

class MIDIPlayerConfig {
public:
    MIDIPlayerConfig(MIDIPlayer& player);

    bool reload(std::string const& path);
    void display_help() const;
    void update();

    struct ParticlePhysics {
        float x_drag;
        float temperature_multiplier;
        float gravity;
        float temperature_decay;
    };

    std::string display_font() const { return m_properties.display_font; }
    sf::Color default_color() const { return m_properties.default_color; }
    sf::Color background_color() const { return m_properties.background_color; }
    sf::Color overlay_color() const { return m_properties.overlay_color; }
    int particle_count() const { return m_properties.particle_count; }
    float particle_radius() const { return m_properties.particle_radius; }
    float particle_glow_size() const { return m_properties.particle_glow_size; }
    auto dust_physics() const { return m_properties.dust_physics; }
    float smoke_alpha_mul() const { return m_properties.smoke_alpha_mul; }
    float smoke_size_mul() const { return m_properties.smoke_size_mul; }
    auto smoke_physics() const { return m_properties.smoke_physics; }
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
        sf::Color default_color { 111, 198, 114 };
        Config::AnimatableProperty<sf::Color> background_color { sf::Color(7,5,15) };
        sf::Color overlay_color { 2, 3, 4 };
        int particle_count = 2;
        float particle_radius = 0.75;
        float particle_glow_size = 0.04;
        ParticlePhysics dust_physics = {
            .x_drag = 1.01,
            .temperature_multiplier = 0.0000004,
            .gravity = 0.00004,
            .temperature_decay = 0.98,
        };
        float smoke_alpha_mul = 0.01;
        float smoke_size_mul = 3;
        ParticlePhysics smoke_physics = {
            .x_drag = 1.01,
            .temperature_multiplier = 0.0000008,
            .gravity = 0.00004,
            .temperature_decay = 0.99,
        };
        size_t max_events_per_track = 4096;
        Config::AnimatableProperty<double> scale { 0.02 };
        int label_font_size = 50;
        int label_fade_time = 30;
        Config::AnimatableProperty<AnimatableBackground> background_image;
    } m_properties;
};
