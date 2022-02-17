#pragma once

#include <type_traits>
namespace Config
{

template<class T>
class AnimatableProperty
{
public:
    AnimatableProperty() = default;

    AnimatableProperty(T&& t)
    : m_current_value(t) {}

    void set_value_with_factor(T value, double factor)
    {
        m_next_value = value;
        m_factor = factor;
        if(factor == 1)
            m_current_value = m_next_value;
    }

    T value() const
    {
        return m_current_value * (1 - m_factor) + m_next_value * m_factor;
    }

    operator T() const { return value(); }

private:
    T m_next_value {};
    T m_current_value {};
    double m_factor = 0;
};

}
