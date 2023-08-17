#pragma once

#include "CoordinateSystem.hpp"
#include "Random.hpp"
#include <concepts>
#include <limits>
#include <random>
#include <utility>

#pragma GCC optimize("O3")

namespace Util {

class PerlinNoise {
public:
    using Point = Util::Point2f;
    using Pointi = Util::Point2i;
    using Vector = Util::Vector2f;
    using Vectori = Util::Vector2i;

    explicit PerlinNoise(uint64_t seed)
        : m_seed(seed)
    {
    }

    Vector sample_gradient(Point const& coords) const
    {
        // return gradient_at(coords.floored().cast<int>());
        auto pos0 = coords.floored().template cast<int>();
        auto weights = coords - pos0.cast<float>();
        auto grad00 = gradient_at(pos0 + Vectori(0, 0));
        auto grad01 = gradient_at(pos0 + Vectori(0, 1));
        auto grad10 = gradient_at(pos0 + Vectori(1, 0));
        auto grad11 = gradient_at(pos0 + Vectori(1, 1));
        auto i0 = lerp_vec(grad00, grad01, weights.y());
        auto i1 = lerp_vec(grad10, grad11, weights.y());
        return lerp_vec(i0, i1, weights.x());
    }

private:
    static Vector lerp_vec(Vector start, Vector end, float x)
    {
        auto l = std::lerp(start.length(), end.length(), x);
        auto a = std::lerp(start.angle().rad(), end.angle().rad(), x);
        return Vector::create_polar(Util::Angle::radians(a), l);
    }

    Xorshift random_at(Pointi const& coords) const
    {
        uint64_t seed = m_seed;
        for (size_t s = 0; s < 2; s++) {
            seed += coords.component(s);
            seed *= 3147362902385180077;
        }
        return Xorshift { seed };
    }

    Vector gradient_at(Pointi const& coords) const
    {
        auto rng = random_at(coords);
        auto dist = FastNormalDistribution<float>(0, 0.5);
        while (true) {
            Vector result;
            for (size_t s = 0; s < 2; s++) {
                result.set_component(s, dist(rng));
            }
            auto length = result.length();
            // Reject points outside of unit sphere
            if (length <= 1) {
                return result / length;
            }
        }
    }

    float dot_at(Pointi grid0, Point coords) const
    {
        auto grad = gradient_at(grid0);
        return grad.dot(grid0.template cast<float>() - coords);
    }

    uint64_t m_seed = 0;
};

}
