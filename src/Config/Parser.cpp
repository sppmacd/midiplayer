#include "Parser.h"

#include "../Logger.h"
#include "../Try.h"
#include <fmt/color.h>
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace Config
{

std::optional<int> PropertyParser::read_int()
{
    m_in >> std::ws;
    int value;
    if(m_in >> value)
        return value;
    logger::error("expected int");
    return {};
}
std::optional<int> PropertyParser::read_int_in_range(int min, int max)
{
    int value = TRY_OPTIONAL(read_int());
    if(value < min || value > max)
    {
        logger::error("value must be in range <{},{}>, {} given", min, max, value);
        return {};
    }
    return value;
}

std::optional<float> PropertyParser::read_float()
{
    m_in >> std::ws;
    float value;
    if(m_in >> value)
        return value;
    logger::error("expected float");
    return {};
}

std::optional<std::string> PropertyParser::read_string()
{
    m_in >> std::ws;
    char maybe_quot = m_in.peek();

    auto read_quotted = [&](char q) -> std::optional<std::string>
    {
        // first quot
        m_in.get();

        // TODO: Handle escape sequences
        std::string result;
        while(m_in.peek() != q)
        {
            result += m_in.get();
            if(!m_in.good())
            {
                logger::error("failed to read next character");
                return {};
            }
        }
        // last quot
        if(m_in.get() != q)
        {
            logger::error("unclosed string literal");
            return {};
        }
        return result;
    };

    if(maybe_quot == '\'' || maybe_quot == '"')
        return read_quotted(maybe_quot);

    std::string result;
    if(m_in >> result)
        return result;

    logger::error("expected string");
    return {};
}

std::optional<sf::Color> PropertyParser::read_color(ColorAlphaMode alpha_mode)
{
    m_in >> std::ws;

    // TODO: Support hex (HTML) colors
    int r, g, b, a = 255;
    if(!(m_in >> r >> g >> b))
    {
        logger::error("color requires <r> <g> <b> components");
        return {};
    }
    if(r > 255 || g > 255 || b > 255 || r < 0 || g < 0 || b < 0)
    {
        logger::error("color requires components in range <0;255>, given ({}, {}, {})", r, g, b);
        return {};
    }
    switch(alpha_mode)
    {
        case ColorAlphaMode::DontAllow:
            return sf::Color(r, g, b, a);
        case ColorAlphaMode::Allow:
        {
            if(!(m_in >> a))
            {
                m_in.clear();
                return sf::Color(r, g, b, 255);
            }
            if(a > 255 || a < 0)
            {
                logger::error("color requires alpha component in range <0;255>, given {}", a);
                return {};
            }
            return sf::Color(r, g, b, a);
        }
        case ColorAlphaMode::Require:
        {
            if(!(m_in >> a))
            {
                logger::error("color requires alpha component");
                return {};
            }
            if(a > 255 || a < 0)
            {
                logger::error("color requires alpha component in range <0;255>, given {}", a);
                return {};
            }
            m_in.clear();
            return sf::Color(r, g, b, a);
        }
    }
    return {};
}

std::optional<SelectorList> PropertyParser::read_selector_list()
{
    SelectorList selectors;
    while(auto selector = Selector::read(m_in))
    {
        if(!selector && selectors.empty())
        {
            logger::error("invalid selector");
            return {};
        }
        selectors.push_back(std::move(selector));
    }
    return selectors;
}

std::optional<std::vector<PropertyParameter>> PropertyParser::parse_property_parameters(std::span<PropertyFormalParameter const> params)
{
    std::vector<PropertyParameter> result;
    for(auto const& formal_param : params)
    {
        auto param = TRY_OPTIONAL([&]() -> std::optional<PropertyParameter>
            {
                std::cerr << (int)formal_param.type() << std::endl;
                switch(formal_param.type())
                {
                    case PropertyType::Int:
                        return TRY_OPTIONAL(read_int());
                    case PropertyType::Float:
                        return TRY_OPTIONAL(read_float());
                    case PropertyType::String:
                        return TRY_OPTIONAL(read_string());
                    case PropertyType::ColorRGB:
                        return TRY_OPTIONAL(read_color(ColorAlphaMode::DontAllow));
                    case PropertyType::ColorRGBA:
                        return TRY_OPTIONAL(read_color(ColorAlphaMode::Allow));
                    case PropertyType::SelectorList:
                        return TRY_OPTIONAL(read_selector_list());
                    default:
                        return {};
                }
            }());
        if(!formal_param.value_matches(param))
        {
            logger::error("Value for parameter '{}' must match {}", formal_param.name(), formal_param.match_expression()->to_string());
            return {};
        }
        result.push_back(std::move(param));
    }
    assert(result.size() == params.size());
    return result;
}

void ConfigFileReader::register_property(std::string name, std::string description, std::vector<PropertyFormalParameter> parameters, OnSetPropertyFunction on_set_property)
{
    m_properties.emplace(std::move(name), Property { std::move(description), std::move(parameters), std::move(on_set_property) });
}

bool ConfigFileReader::load(std::string const& file_name)
{
    std::ifstream config_file(file_name);
    if(config_file.fail())
        return false;

    auto print_next_line = [&]()
    {
        std::string line;
        for(size_t s = 0; s < 2; s++)
        {
            if(!std::getline(config_file, line))
            {
                std::cerr << "(end of file)" << std::endl;
                return;
            }
        }
        std::cerr << "| " << line << std::endl;
    };

    while(true)
    {
        std::string property_name;
        if(!(config_file >> property_name))
            break;
        if(property_name.starts_with('#'))
        {
            config_file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        auto property = m_properties.find(property_name);
        if(property == m_properties.end())
        {
            logger::error("unknown property: '{}', see --config-help for available properties", property_name);
            return false;
        }
        PropertyParser parser { config_file };
        auto property_parameters = parser.parse_property_parameters(property->second.parameters);
        if(!property_parameters.has_value() || !property->second.on_set_property(property_parameters.value()))
        {
            // TODO: Display line:column
            logger::error_note("in property: '{}', offset {}", property_name, config_file.tellg());
            print_next_line();
            return false;
        }
    }
    return true;
}

std::string ConfigFileReader::Property::get_usage_string() const
{
    std::string result;
    for(size_t s = 0; s < parameters.size(); s++)
    {
        auto& param = parameters[s];
        result += "<";
        result += param.name();
        result += ": ";
        result += param.type_string();
        if(param.match_expression())
            result += "(" + param.match_expression()->to_string() + ")";
        result += ">";
        if(s != parameters.size() - 1)
            result += " ";
    }
    return result;
}

void ConfigFileReader::display_help() const
{
    // TODO: Check for escape sequence support
    std::cerr << "\e[1;33m-- Config File Help --\e[0m" << std::endl
              << std::endl;
    for(auto& it : m_properties)
    {
        std::ostringstream oss;
        // TODO: Usage
        oss << "- \e[1;32m" << it.first << "\e[0;3m "
            << it.second.get_usage_string()
            << "\e[0m";
        std::cerr << std::left << std::setw(75) << oss.str() << " " << it.second.description << std::endl;
    }
}

void ConfigFileReader::set_property(std::string const& name, ArgumentList const& args)
{
    m_properties[name].on_set_property(args);
}

}
