#include "Info.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace Config
{

void Info::register_property(std::string name, std::string description, std::vector<PropertyFormalParameter> parameters, OnSetPropertyFunction on_set_property)
{
    m_properties.emplace(std::move(name), Property { std::move(description), std::move(parameters), std::move(on_set_property) });
}

std::string Info::Property::get_usage_string() const
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

void Info::display_help() const
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

void Info::set_property(std::string const& name, ArgumentList const& args) const
{
    auto it = m_properties.find(name);
    if(it == m_properties.end())
        return;
    it->second.on_set_property(args);
}

std::span<PropertyFormalParameter const> Info::property_formal_parameters(std::string const& name) const
{
    auto it = m_properties.find(name);
    if(it == m_properties.end())
        return {};
    return it->second.parameters;
}

}
