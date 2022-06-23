#pragma once

#include <SFML/Graphics.hpp>
#include <fmt/format.h>
#include <type_traits>

namespace Config {

inline sf::Color operator*(sf::Color const& lhs, double rhs)
{
    return { static_cast<sf::Uint8>(lhs.r * rhs), static_cast<sf::Uint8>(lhs.g * rhs), static_cast<sf::Uint8>(lhs.b * rhs), lhs.a };
}

template<class T>
class AnimatableProperty {
public:
    AnimatableProperty() = default;

    explicit AnimatableProperty(T&& t)
        : m_current_value(t)
    {
    }

    void set_value_with_factor(T value, double factor)
    {
        m_next_value = value;
        m_factor = factor;
        if (factor == 1)
            m_current_value = m_next_value;
    }

    using ValueType = decltype(T() + T());

    auto value() const
    {
        auto v = m_current_value * (1 - m_factor) + m_next_value * m_factor;
        return v;
    }

    operator ValueType() const { return value(); }

private:
    T m_next_value {};
    T m_current_value {};
    double m_factor = 0;
};

}
