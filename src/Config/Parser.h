#pragma once

#include "Property.h"
#include "Selector.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>

namespace Config
{

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

    // [name=value]...
    std::optional<SelectorList> read_selector_list();

    std::optional<std::vector<PropertyParameter>> parse_property_parameters(std::span<PropertyFormalParameter const>);

private:
    friend class ConfigFileReader;

    PropertyParser(std::ifstream& in)
    : m_in(in) {}

    std::ifstream& m_in;
};

class ConfigFileReader
{
public:
    using OnSetPropertyFunction = std::function<bool(ArgumentList const&)>;

    void register_property(std::string name, std::string description, std::vector<PropertyFormalParameter> parameters, OnSetPropertyFunction on_set_property);
    bool load(std::string const& file_name);
    void display_help() const;

    void set_property(std::string const& name, ArgumentList const& args);

private:
    struct Property
    {
        std::string description;
        std::vector<PropertyFormalParameter> parameters;
        OnSetPropertyFunction on_set_property;

        std::string get_usage_string() const;
    };
    std::map<std::string, Property> m_properties;
};

}
