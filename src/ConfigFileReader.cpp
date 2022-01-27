#include "ConfigFileReader.h"

#include "Try.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

std::optional<int> PropertyParser::read_int()
{
    m_in >> std::ws;
    int value;
    if(m_in >> value)
        return value;
    std::cerr << "ERROR: expected int" << std::endl;
    return {};
}
std::optional<int> PropertyParser::read_int_in_range(int min, int max)
{
    int value = TRY_OPTIONAL(read_int());
    if(value < min || value > max)
    {
        std::cerr << "ERROR: value must be in range <" << min << "," << max << ">, " << value << " given" << std::endl;
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
    std::cerr << "ERROR: expected float" << std::endl;
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
                std::cerr << "ERROR: failed to read next character" << std::endl;
                return {};
            }
        }
        // last quot
        if(m_in.get() != q)
        {
            std::cerr << "ERROR: unclosed string literal" << std::endl;
            return {};
        }
        return result;
    };

    if(maybe_quot == '\'' || maybe_quot == '"')
        return read_quotted(maybe_quot);

    std::string result;
    if(m_in >> result)
        return result;

    std::cerr << "ERROR: expected string" << std::endl;
    return {};
}

std::optional<sf::Color> PropertyParser::read_color(ColorAlphaMode alpha_mode)
{
    m_in >> std::ws;

    // TODO: Support hex (HTML) colors
    int r, g, b, a = 255;
    if(!(m_in >> r >> g >> b))
    {
        std::cerr << "ERROR: color requires <r> <g> <b> components" << std::endl;
        return {};
    }
    if(r > 255 || g > 255 || b > 255 || r < 0 || g < 0 || b < 0)
    {
        std::cerr << "ERROR: color requires components in range <0;255>, given (" << r << ", " << g << ", " << b << ")" << std::endl;
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
                std::cerr << "ERROR: color requires alpha component in range <0;255>, given " << a << std::endl;
                return {};
            }
            return sf::Color(r, g, b, a);
        }
        case ColorAlphaMode::Require:
        {
            if(!(m_in >> a))
            {
                std::cerr << "ERROR: color requires alpha component" << std::endl;
                return {};
            }
            if(a > 255 || a < 0)
            {
                std::cerr << "ERROR: color requires alpha component in range <0;255>, given " << a << std::endl;
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
            std::cerr << "ERROR: invalid selector" << std::endl;
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
                std::cerr << "(end of file)" << std::endl;
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
            std::cerr << "ERROR: unknown property: '" << property_name << "'" << std::endl;
            display_help();
            return false;
        }
        PropertyParser parser { config_file };
        if(!property->second.handler(parser))
        {
            // TODO: Display line:column
            std::cerr << "ERROR:   in property: '" << property_name << "', offset " << config_file.tellg() << std::endl;
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
    std::cerr << "\e[1;33m-- Config File Help --\e[0m" << std::endl << std::endl;
    for(auto& it : m_properties)
    {
        std::cerr << "- \e[1;32m" << it.first << "\e[0m - " << it.second.description << std::endl;
        std::cerr << "   Usage: \e[32m" << it.first << "\e[0;3m " << it.second.usage << "\e[0m" << std::endl;
    }
}
