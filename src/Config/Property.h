#pragma once

#include "AttributeValue.h"
#include "Selector.h"
#include "Time.h"
#include <SFML/Graphics.hpp>
#include <cassert>
#include <optional>
#include <string>
#include <variant>

namespace Config
{

class PropertyParameter;

enum class PropertyType
{
    Invalid,
    Int,
    Float,
    String,
    ColorRGB,
    ColorRGBA,
    SelectorList,
    Time
};

class PropertyFormalParameter
{
public:
    /// \param name is used only for help string
    // FIXME: Figure out how to get rid of this shared_ptr
    PropertyFormalParameter(PropertyType type, std::string_view name, std::shared_ptr<MatchExpression> match_expression = nullptr)
    : m_type(type), m_name(name), m_match_expression(std::move(match_expression)) {}

    PropertyType type() const { return m_type; }
    std::string_view name() const { return m_name; }
    std::string_view type_string() const;
    MatchExpression const* match_expression() const { return m_match_expression.get(); }
    bool value_matches(PropertyParameter const&) const;

private:
    PropertyType m_type { PropertyType::Invalid };
    std::string_view m_name;
    std::shared_ptr<MatchExpression> m_match_expression;
};

using SelectorList = std::vector<std::shared_ptr<Selector>>;
using PropertyParameterBase = std::variant<int, float, std::string, sf::Color, SelectorList, Time>;

class PropertyParameter : public PropertyParameterBase
{
public:
    template<class T>
    PropertyParameter(T&& value)
    : PropertyParameterBase(std::forward<T>(value)) {}

    float as_int() const
    {
        assert(std::holds_alternative<int>(*this));
        return std::get<int>(*this);
    }
    float as_float() const
    {
        assert(std::holds_alternative<float>(*this));
        return std::get<float>(*this);
    }
    std::string const& as_string() const
    {
        assert(std::holds_alternative<std::string>(*this));
        return std::get<std::string>(*this);
    }
    sf::Color const& as_color() const
    {
        assert(std::holds_alternative<sf::Color>(*this));
        return std::get<sf::Color>(*this);
    }
    SelectorList const& as_selector_list() const
    {
        assert(std::holds_alternative<SelectorList>(*this));
        return std::get<SelectorList>(*this);
    }
    Time const& as_time() const
    {
        assert(std::holds_alternative<Time>(*this));
        return std::get<Time>(*this);
    }
};

class ArgumentList
{
public:
    ArgumentList(std::vector<PropertyParameter> args)
    : m_args(args) {}

    PropertyParameter const& operator[](size_t i) const
    {
        assert(i < m_args.size());
        return m_args[i];
    }

private:
    std::vector<PropertyParameter> m_args;
};

}
