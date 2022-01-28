#include "ConfigFileReader.h"

#include "Logger.h"
#include "Try.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

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

std::optional<std::vector<std::unique_ptr<Selector>>> PropertyParser::read_selector_list()
{
    std::vector<std::unique_ptr<Selector>> selectors;
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

void ConfigFileReader::register_property(std::string name, std::string description, std::string help_string, PropertyHandler handler)
{
    m_properties.insert({ name, { description, help_string, std::move(handler) } });
}

bool ConfigFileReader::load(std::string const& file_name)
{
    std::ifstream config_file(file_name);
    if(config_file.fail())
    {
        std::cerr << "WARNING: " << file_name << " doesn't exist." << std::endl;
        return false;
    }

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
        if(!property->second.handler(parser))
        {
            // TODO: Display line:column
            logger::error_note("in property: '{}', offset {}", property_name, config_file.tellg());
            print_next_line();
            std::cerr << "Usage: " << property_name << " " << property->second.usage << std::endl;
            return false;
        }
    }
    return true;
}

void ConfigFileReader::display_help() const
{
    // TODO: Check for escape sequence support
    std::cerr << "\e[1;33m-- Config File Help --\e[0m" << std::endl
              << std::endl;
    for(auto& it : m_properties)
    {
        std::ostringstream oss;
        oss << "- \e[1;32m" << it.first << "\e[0;3m " << it.second.usage << "\e[0m"
            << "\e[0m";
        std::cerr << std::left << std::setw(75) << oss.str() << " " << it.second.description << std::endl;
    }
}
