#include "Config.h"

#include "Logger.h"
#include "Try.h"

Config::Config()
{
    m_reader.register_property("color", "Key tile color", "<selectors(Selector)...> <color(rgb[a])>", [&](PropertyParser& parser) -> bool
        { 
        auto selectors = TRY_OPTIONAL(parser.read_selector_list());
        auto color = TRY_OPTIONAL(parser.read_color(PropertyParser::ColorAlphaMode::Allow));
        m_options.channel_colors.push_back(std::make_pair(std::move(selectors), color));
        return true; });
    m_reader.register_property("default_color", "Default key tile color. Used when no selector matches a tile.", "<color(rgb[a])>", [&](PropertyParser& parser) -> bool
        { 
        m_options.default_color = TRY_OPTIONAL(parser.read_color(PropertyParser::ColorAlphaMode::Allow));
        return true; });
    m_reader.register_property("background_color", "Background color", "<color(rgb)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.background_color = TRY_OPTIONAL(parser.read_color(PropertyParser::ColorAlphaMode::DontAllow));
        return true; });
    m_reader.register_property("overlay_color", "Overlay (fade out) color", "<color(rgb[a])>", [&](PropertyParser& parser) -> bool
        { 
        m_options.overlay_color = TRY_OPTIONAL(parser.read_color(PropertyParser::ColorAlphaMode::Allow));
        return true; });
    m_reader.register_property("particle_count", "Particle count (per tick)", "<count(int)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.particle_count = TRY_OPTIONAL(parser.read_int());
        return true; });
    m_reader.register_property("particle_radius", "Particle radius (in keys)", "<radius(float)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.particle_radius = TRY_OPTIONAL(parser.read_float());
        return true; });
    m_reader.register_property("particle_glow_size", "Particle glow size (in keys)", "<radius(float)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.particle_glow_size = TRY_OPTIONAL(parser.read_float());
        return true; });
    m_reader.register_property("max_events_per_track", "Maximum events that are stored in track. Applicable only for realtime mode.", "<count(int)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.max_events_per_track = TRY_OPTIONAL(parser.read_int_in_range(0, 65536));
        return true; });
    m_reader.register_property("real_time_scale", "Y scale (tile falling speed) for realtime mode", "<value(float)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.real_time_scale = TRY_OPTIONAL(parser.read_float());
        return true; });
    m_reader.register_property("play_scale", "Y scale (tile falling speed) for play mode", "<value(float)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.play_scale = TRY_OPTIONAL(parser.read_float());
        return true; });
    m_reader.register_property("background_image", "Path to background image", "<path(string)>", [&](PropertyParser& parser) -> bool
        { 
        m_options.background_image = TRY_OPTIONAL(parser.read_string());
        return true; });
    m_reader.register_property("display_font", "Font used for displaying e.g. labels", "<path(string)>", [&](PropertyParser& parser) -> bool
        {
        m_options.display_font = TRY_OPTIONAL(parser.read_string());
        return true; });
    m_reader.register_property("label_font_size", "Font size for labels (in pt)", "<size(int)>", [&](PropertyParser& parser) -> bool
        {
        m_options.label_font_size = TRY_OPTIONAL(parser.read_int_in_range(1, 1000));
        return true; });
    m_reader.register_property("label_fade_time", "Label fade time (in frames)", "<time(int)>", [&](PropertyParser& parser) -> bool
        {
        m_options.label_fade_time = TRY_OPTIONAL(parser.read_int_in_range(1, 1000));
        return true; });
}

bool Config::reload(std::string const& path)
{
    m_options = {};
    return m_reader.load(path);
}

sf::Color Config::resolve_color(MIDIPlayer const& player, NoteEvent& event) const
{
    for(auto const& pair : m_options.channel_colors)
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
    return m_options.default_color;
}
