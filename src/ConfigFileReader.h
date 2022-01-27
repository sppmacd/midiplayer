#pragma once

#include "Selector.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>

class PropertyParser
{
public:
    std::optional<int> read_int();
    std::optional<int> read_int_in_range(int min, int max);
    std::optional<float> read_float();
    std::optional<std::string> read_string();

    enum class ColorAlphaMode
    {
        DontAllow, // #(<r> <g> <b>)
        Allow,     // #(<r> <g> <b> [a])
        Require    // #(<r> <g> <b> <a>)
    };

    std::optional<sf::Color> read_color(ColorAlphaMode);

    // [...]
    std::optional<std::vector<std::unique_ptr<Selector>>> read_selector_list();

private:
    friend class ConfigFileReader;

    PropertyParser(std::ifstream& in)
    : m_in(in) {}

    std::ifstream& m_in;
};

class ConfigFileReader
{
public:
    using PropertyHandler = std::function<bool(PropertyParser&)>;

    void register_property(std::string name, std::string description, std::string usage, PropertyHandler handler);
    bool load(std::string const& file_name);
    void display_help() const;

private:
    struct Property
    {
        std::string description;
        std::string usage;
        PropertyHandler handler;
    };
    std::map<std::string, Property> m_properties;
};
