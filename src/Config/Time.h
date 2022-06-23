#pragma once

namespace Config {

class Time {
public:
    enum class Unit {
        Seconds,
        Ticks,
        Frames,
    };

    Time() = default;

    Time(double value, Unit unit)
        : m_value(value)
        , m_unit(unit)
    {
    }

    double value() const { return m_value; }
    Unit unit() const { return m_unit; }

private:
    double m_value;
    Unit m_unit;
};

}
