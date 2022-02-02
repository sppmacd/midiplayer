#include "Config.h"

#include "Config/Property.h"
#include "Logger.h"
#include "Try.h"

namespace Config
{

Config::Config()
{
    // FIXME: There is a copy here which could be omitted, but this makes shared_ptr needed.
    m_reader.register_property("color",
        "Key tile color, applied only to tiles matching `selector`",
        { { PropertyType::SelectorList, "selectors" }, { PropertyType::ColorRGBA, "color" } },
        [&](ArgumentList const& arglist) -> bool
        {
            auto& selectors = arglist[0].as_selector_list();
            auto color = arglist[1].as_color();
            m_properties.channel_colors.push_back({ std::move(selectors), color });
            return true;
        });
    m_reader.register_property("default_color",
        "Default key tile color. Used when no selector matches a tile.",
        { { PropertyType::ColorRGBA, "color" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.default_color = arglist[0].as_color();
            return true;
        });
    m_reader.register_property("background_color",
        "Background color",
        { { PropertyType::ColorRGB, "color" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.background_color = arglist[0].as_color();
            return true;
        });
    m_reader.register_property("overlay_color",
        "Overlay (fade out) color",
        { { PropertyType::ColorRGBA, "color" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.overlay_color = arglist[0].as_color();
            return true;
        });
    m_reader.register_property("particle_count",
        "Particle count (per tick)",
        { { PropertyType::Int, "count" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.particle_count = arglist[0].as_int();
            return true;
        });
    m_reader.register_property("particle_radius",
        "Particle radius (in keys)",
        { { PropertyType::Float, "radius" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.particle_radius = arglist[0].as_float();
            return true;
        });
    m_reader.register_property("particle_glow_size",
        "Particle glow size (in keys)",
        { { PropertyType::Float, "radius" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.particle_glow_size = arglist[0].as_float();
            return true;
        });
    m_reader.register_property("max_events_per_track",
        "Maximum events that are stored in track. Applicable only for realtime mode.",
        { { PropertyType::Int, "count" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.max_events_per_track = arglist[0].as_int();
            return true;
        });
    m_reader.register_property("real_time_scale",
        "Y scale (tile falling speed) for realtime mode",
        { { PropertyType::Float, "value" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.real_time_scale = arglist[0].as_float();
            return true;
        });
    m_reader.register_property("play_scale",
        "Y scale (tile falling speed) for play mode",
        { { PropertyType::Float, "value" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.play_scale = arglist[0].as_float();
            return true;
        });
    m_reader.register_property("background_image",
        "Path to background image",
        { { PropertyType::String, "path" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.background_image = arglist[0].as_string();
            return true;
        });
    m_reader.register_property("display_font",
        "Path to font used for displaying e.g. labels",
        { { PropertyType::String, "path" } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.display_font = arglist[0].as_string();
            return true;
        });
    m_reader.register_property("label_font_size",
        "Font size for labels (in pt)",
        { { PropertyType::Int, "size", std::make_shared<Range>(1, 1000) } },
        [&](ArgumentList const& arglist) -> bool
        {
            m_properties.label_font_size = arglist[0].as_int();
            return true;
        });
    m_reader.register_property("label_fade_time",
        "Label fade time (in frames)",
        { { PropertyType::Int, "time", std::make_shared<Range>(1, 1000) } },
        [&](ArgumentList const& arglist) -> bool
        {
            // FIXME: Allow units
            m_properties.label_font_size = arglist[0].as_int();
            return true;
        });
}

bool Config::reload(std::string const& path)
{
    m_properties = {};
    return m_reader.load(path);
}

sf::Color Config::resolve_color(MIDIPlayer const& player, NoteEvent& event) const
{
    for(auto const& pair : m_properties.channel_colors)
    {
        bool matched = true;
        for(auto const& selector : pair.first)
        {
            if(!selector->matches(player, event))
            {
                matched = false;
                break;
            }
        }
        if(matched)
            return pair.second;
    }
    return m_properties.default_color;
}

void Config::set_property(std::string const& name, std::vector<PropertyParameter> const& params)
{
    m_reader.set_property(std::move(name), params);
}

}
