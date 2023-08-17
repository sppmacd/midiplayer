// Taken from EssaGUI (https://github.com/essa-software/EssaGUI)
// Copyright (c) 2023, Essa Software
// SPDX-License-Identifier: Beerware

#pragma once

#include "CoordinateSystem/Interoperability.hpp"

// clang-format off
#define CREATE_TYPED_ALIASES(Tp)        \
    template<class T>                   \
    using Tp##2 = Detail::Tp<2, T>;     \
    using Tp##2i = Tp##2<int>;          \
    using Tp##2u = Tp##2<unsigned>;     \
    using Tp##2f = Tp##2<float>;        \
    using Tp##2d = Tp##2<double>;       \
                                        \
    template<class T>                   \
    using Tp##3 = Detail::Tp<3, T>;     \
    using Tp##3i = Tp##3<int>;          \
    using Tp##3u = Tp##3<unsigned>;     \
    using Tp##3f = Tp##3<float>;        \
    using Tp##3d = Tp##3<double>;       \
                                        \
    template<class T>                   \
    using Tp##4 = Detail::Tp<4, T>;     \
    using Tp##4i = Tp##4<int>;          \
    using Tp##4u = Tp##4<unsigned>;     \
    using Tp##4f = Tp##4<float>;        \
    using Tp##4d = Tp##4<double>;
// clang-format on

namespace Util {

CREATE_TYPED_ALIASES(Point)
CREATE_TYPED_ALIASES(Size)
CREATE_TYPED_ALIASES(Vector)
#undef CREATE_TYPED_ALIASES

}
