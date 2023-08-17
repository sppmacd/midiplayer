// Taken from EssaGUI (https://github.com/essa-software/EssaGUI)
// Copyright (c) 2023, Essa Software
// SPDX-License-Identifier: Beerware

#pragma once

#include "Coordinates.hpp"
#include <fmt/core.h>

#ifdef ESSA_COMPILER_GCC
#    pragma GCC optimize("O3")
#endif

namespace Util {

namespace Detail {

template<size_t C, class T> class Vector;

template<size_t C, class T> class Size : public Coordinates<C, T, Size> {
public:
    using Super = Coordinates<C, T, Size>;
    using ThisVector = Vector<C, T>;

    // -> Interoperability
    // Return top-left to bottom-right diagonal vector, effectively converting
    // this's components to vector without change.
    ThisVector to_vector() const;

    template<class... Args>
        requires requires(Args... a) { Super(std::forward<Args>(a)...); }
    Size(Args... a)
        : Super(std::forward<Args>(a)...) { }

    auto diagonal_squared() const {
        double result = 0;
        for (size_t s = 0; s < Super::Components; s++) {
            result += this->component(s) * this->component(s);
        }
        return result;
    }

    auto diagonal() const { return std::sqrt(diagonal_squared()); }

    // Returns ratio between x and y coordinate.
    constexpr float aspect_ratio() const
        requires(Super::Components == 2)
    {
        return static_cast<float>(this->x()) / this->y();
    }

    constexpr Size operator+(ThisVector const& b) const {
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, this->component(s) + b.component(s));
        }
        return ab;
    }

    constexpr Size& operator+=(ThisVector const& b) { return *this = *this + b; }

    constexpr Size operator-(ThisVector const& b) const {
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, this->component(s) - b.component(s));
        }
        return ab;
    }

    constexpr Size& operator-=(ThisVector const& b) { return *this = *this - b; }

    constexpr Size operator-(Size const& b) const {
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, this->component(s) - b.component(s));
        }
        return ab;
    }

    constexpr Size& operator-=(Size const& b) { return *this = *this - b; }

    constexpr Size operator*(double x) const {
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, this->component(s) * x);
        }
        return ab;
    }

    constexpr Size& operator*=(double x) { return *this = *this * x; }

    constexpr Size operator/(double x) const {
        assert(x != 0);
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, this->component(s) / x);
        }
        return ab;
    }

    constexpr Size& operator/=(double x) { return *this = *this / x; }

    constexpr Size operator-() const {
        Size ab;
        for (size_t s = 0; s < Super::Components; s++) {
            ab.set_component(s, -this->component(s));
        }
        return ab;
    }

    //// Size2 ////
    template<size_t OtherC, class OtherT>
        requires(Super::Components == 2 && OtherC >= 2)
    constexpr explicit Size(Vector<OtherC, OtherT> other)
        : Size { other.x(), other.y() } { }

    //// Size3 ////
    template<size_t OtherC, class OtherT>
        requires(Super::Components == 3 && OtherC >= 3)
    constexpr explicit Size(Vector<OtherC, OtherT> other)
        : Size { other.x(), other.y(), other.z() } { }

    //// Size4 ////

    template<size_t OtherC, class OtherT>
        requires(Super::Components == 4 && OtherC >= 4)
    constexpr explicit Size(Vector<OtherC, OtherT> other)
        : Size { other.x(), other.y(), other.z(), other.w() } { }

    bool operator==(Size const&) const = default;
};

} // Detail

template<size_t C, class T> Detail::Size<C, T> operator*(double fac, Detail::Size<C, T> const& vec) { return vec * fac; }

} // Util

template<size_t C, class T> class fmt::formatter<Util::Detail::Size<C, T>> : public fmt::formatter<T> {
public:
    template<typename FormatContext> constexpr auto format(Util::Detail::Size<C, T> const& v, FormatContext& ctx) const {
        for (size_t s = 0; s < C; s++) {
            ctx.advance_to(fmt::formatter<T>::format(v.component(s), ctx));
            if (s != C - 1)
                fmt::format_to(ctx.out(), "x");
        }
        return ctx.out();
    }
};
