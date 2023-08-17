// Taken from EssaGUI (https://github.com/essa-software/EssaGUI)
// Copyright (c) 2023, Essa Software
// SPDX-License-Identifier: Beerware

#pragma once

#include "Point.hpp"
#include "Size.hpp"
#include "Vector.hpp"

namespace Util::Detail {

template<size_t C, class T> Point<C, T> Vector<C, T>::to_point() const { return ThisPoint {} + *this; }

template<size_t C, class T> Vector<C, T> Point<C, T>::to_vector() const { return *this - Point {}; }

template<size_t C, class T> Vector<C, T> Size<C, T>::to_vector() const {
    ThisVector result;
    for (size_t s = 0; s < C; s++) {
        result.set_component(s, this->component(s));
    }
    return result;
}

template<size_t C, class T> Size<C, T> Vector<C, T>::to_size() const {
    ThisSize result;
    for (size_t s = 0; s < C; s++) {
        result.set_component(s, this->component(s));
    }
    return result;
}

template<size_t C, class T>
constexpr Point<C, T> Point<C, T>::create_spheric(Angle lat, Angle lon, double radius)
    requires(C == 3)
{
    return create_spheric({ lat, lon }, radius);
}

template<size_t C, class T>
constexpr Vector<C, T> Vector<C, T>::create_spheric(Angle lat, Angle lon, double radius)
    requires(C == 3)
{
    return create_spheric({ lat, lon }, radius);
}

}
