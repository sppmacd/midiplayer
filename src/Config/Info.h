#pragma once

#include "Property.h"
#include "Time.h"

#include <functional>
#include <span>

namespace Config {

class Info {
public:
    using OnSetPropertyFunction = std::function<bool(ArgumentList const&, double)>;

    void register_property(std::string name, std::string description, std::vector<PropertyFormalParameter> parameters, OnSetPropertyFunction on_set_property);

    void display_help() const;
    void set_property(std::string const& name, ArgumentList const& args, double transition_factor) const;
    std::span<PropertyFormalParameter const> property_formal_parameters(std::string const& name) const;

    auto const& properties() const { return m_properties; }

private:
    struct Property {
        std::string description;
        std::vector<PropertyFormalParameter> parameters;
        OnSetPropertyFunction on_set_property;

        std::string get_usage_string() const;
    };
    std::map<std::string, Property> m_properties;
};

}
