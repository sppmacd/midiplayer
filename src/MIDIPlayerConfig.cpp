#include "MIDIPlayerConfig.h"

#include "Config/Lexer.h"
#include "Config/Parser.h"
#include "Config/Property.h"
#include "Logger.h"
#include "MIDIPlayer.h"
#include "Try.h"

#include <algorithm>
#include <fstream>

MIDIPlayerConfig::MIDIPlayerConfig(MIDIPlayer& player)
: m_reader(m_info, player)
{
    // FIXME: There is a copy here which could be omitted, but this makes shared_ptr needed.
    m_info.register_property("color",
        "Tile color, applied only to tiles matching `selector`.",
        { { Config::PropertyType::SelectorList, "selectors" }, { Config::PropertyType::ColorRGBA, "color" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            auto& selectors = arglist[0].as_selector_list();
            auto color = arglist[1].as_color();
            m_reader.player().add_static_tile_color(selectors, color);
            return true;
        });
    m_info.register_property("animate_color",
        "Apply transition of tile color. Applies only to tiles matching `selector`.",
        { { Config::PropertyType::SelectorList, "selectors" }, { Config::PropertyType::ColorRGBA, "color" } },
        [&](Config::ArgumentList const& arglist, double transition) -> bool
        {
            auto& selectors = arglist[0].as_selector_list();
            auto color = arglist[1].as_color();
            m_reader.player().update_note_transitions(selectors, color, transition);
            return true;
        });
    m_info.register_property("default_color",
        "Default key tile color. Used when no selector matches a tile.",
        { { Config::PropertyType::ColorRGBA, "color" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.default_color = arglist[0].as_color();
            return true;
        });
    m_info.register_property("background_color",
        "Background color",
        { { Config::PropertyType::ColorRGB, "color" } },
        [&](Config::ArgumentList const& arglist, double transition) -> bool
        {
            m_properties.background_color.set_value_with_factor(arglist[0].as_color(), transition);
            return true;
        });
    m_info.register_property("overlay_color",
        "Overlay (fade out) color",
        { { Config::PropertyType::ColorRGBA, "color" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.overlay_color = arglist[0].as_color();
            return true;
        });
    m_info.register_property("particle_count",
        "Particle count (per tick)",
        { { Config::PropertyType::Int, "count" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.particle_count = arglist[0].as_int();
            return true;
        });
    m_info.register_property("particle_radius",
        "Particle radius (in keys)",
        { { Config::PropertyType::Float, "radius" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.particle_radius = arglist[0].as_float();
            return true;
        });
    m_info.register_property("particle_glow_size",
        "Particle glow size (in keys)",
        { { Config::PropertyType::Float, "radius" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.particle_glow_size = arglist[0].as_float();
            return true;
        });
    m_info.register_property("max_events_per_track",
        "Maximum events that are stored in track. Applicable only for realtime mode.",
        { { Config::PropertyType::Int, "count" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.max_events_per_track = arglist[0].as_int();
            return true;
        });
    m_info.register_property("scale",
        "Y scale (tile falling speed)",
        { { Config::PropertyType::Float, "value" } },
        [&](Config::ArgumentList const& arglist, double factor) -> bool
        {
            m_properties.scale.set_value_with_factor(arglist[0].as_float(), factor);
            return true;
        });
    m_info.register_property("background_image",
        "Path to background image",
        { { Config::PropertyType::String, "path" } },
        [&](Config::ArgumentList const& arglist, double transition) -> bool
        {
            auto texture = m_reader.player().get_background_image(arglist[0].as_string());
            m_properties.background_image.set_value_with_factor(AnimatableBackground(texture), transition);
            return true;
        });
    m_info.register_property("display_font",
        "Path to font used for displaying e.g. labels",
        { { Config::PropertyType::String, "path" } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.display_font = arglist[0].as_string();
            return true;
        });
    m_info.register_property("label_font_size",
        "Font size for labels (in pt)",
        { { Config::PropertyType::Int, "size", std::make_shared<Range>(1, 1000) } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            m_properties.label_font_size = arglist[0].as_int();
            return true;
        });
    m_info.register_property("label_fade_time",
        "Label fade time (in frames)",
        { { Config::PropertyType::Int, "time", std::make_shared<Range>(1, 1000) } },
        [&](Config::ArgumentList const& arglist, double) -> bool
        {
            // FIXME: Allow units
            m_properties.label_font_size = arglist[0].as_int();
            return true;
        });
}

void MIDIPlayerConfig::update()
{
    m_reader.update();
}

static void display_source_range(std::ostream& output, std::istream& input, Config::SourceRange const& span)
{
    // TODO: Handle EOF errors
    size_t start = span.location.index - span.location.column + 1;
    input.clear();
    input.seekg(start);

    std::string code;
    if(!std::getline(input, code))
    {
        output << "(failed to read code)" << std::endl;
        return;
    }

    // TODO: Handle multiline
    output << " | " << code << std::endl
           << " | ";
    for(size_t s = 0; s < span.location.column - 1; s++)
        output << " ";

    for(size_t s = 0; s < span.size; s++)
        output << "^";

    output << std::endl;
}

bool MIDIPlayerConfig::reload(std::string const& path)
{
    m_properties = {};
    m_reader.clear();

    std::ifstream file(path);
    if(file.fail())
        return false;
    Config::Lexer lexer { file };
    auto tokens = lexer.lex();
    auto config = Config::Parser::parse(m_info, tokens);
    if(config.has_error())
    {
        logger::error("Failed to parse config file: {}", config.error().message);
        logger::error_note("at {}:{}", config.error().range.location.line, config.error().range.location.column);
        display_source_range(std::cerr, file, config.error().range);
        return false;
    }

    return config.release_value().execute(m_reader);
}

void MIDIPlayerConfig::display_help() const
{
    m_info.display_help();
}

void MIDIPlayerConfig::set_property(std::string const& name, std::vector<Config::PropertyParameter> const& params)
{
    m_info.set_property(std::move(name), params, 1);
}

void MIDIPlayerConfig::dump_stats(std::ostream& out) const
{
    out << "-- Config Reader" << std::endl;
    m_reader.dump_stats(out);
}
