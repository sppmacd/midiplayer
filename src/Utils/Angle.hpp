// Taken from EssaGUI (https://github.com/essa-software/EssaGUI)
// Copyright (c) 2023, Essa Software
// SPDX-License-Identifier: Beerware

#pragma once

#include <cmath>
#include <compare>
#include <fmt/core.h>
#include <ostream>

namespace Util {

template<class T> constexpr T degrees_in_radian = 180 / M_PI;

constexpr auto rad_to_deg(auto radians) { return radians * degrees_in_radian<decltype(radians)>; }

constexpr auto deg_to_rad(auto degrees) { return degrees / degrees_in_radian<decltype(degrees)>; }

class Angle {
public:
    constexpr Angle() = default;

    constexpr static Angle degrees(double d) { return Angle { deg_to_rad(d) }; }
    constexpr static Angle radians(double r) { return Angle { r }; }

    constexpr double deg() const { return rad_to_deg(m_value_in_radians); }
    constexpr double rad() const { return m_value_in_radians; }

    constexpr Angle operator-() const { return Angle(-m_value_in_radians); }

    constexpr Angle operator+(Angle const& other) const { return Angle { m_value_in_radians + other.m_value_in_radians }; }

    constexpr Angle operator-(Angle const& other) const { return Angle { m_value_in_radians - other.m_value_in_radians }; }

    constexpr Angle operator*(double n) const { return Angle { m_value_in_radians * n }; }

    constexpr Angle& operator+=(Angle const& other) { return *this = *this + other; }

    constexpr Angle& operator-=(Angle const& other) { return *this = *this - other; }

    constexpr Angle& operator*=(double n) { return *this = *this * n; }

    constexpr std::partial_ordering operator<=>(Angle const& other) const { return m_value_in_radians <=> other.m_value_in_radians; }
    constexpr bool operator==(Angle const& other) const = default;

    friend std::ostream& operator<<(std::ostream& out, Angle alfa) { return out << alfa.rad() << " [rad]"; }

private:
    constexpr Angle(double rads)
        : m_value_in_radians(rads) { }

    double m_value_in_radians = 0;
};

}

constexpr Util::Angle operator""_deg(long double v) { return Util::Angle::degrees(v); }
constexpr Util::Angle operator""_deg(unsigned long long v) { return Util::Angle::degrees(v); }

constexpr Util::Angle operator""_rad(long double v) { return Util::Angle::radians(v); }
constexpr Util::Angle operator""_rad(unsigned long long v) { return Util::Angle::radians(v); }

template<> class fmt::formatter<Util::Angle> : public fmt::formatter<std::string_view> {
public:
    template<typename FormatContext> constexpr auto format(Util::Angle const& p, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}°", p.deg());
    }
};
