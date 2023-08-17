#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>

#pragma GCC optimize("O3")

namespace Util {

class Xorshift {
public:
    explicit Xorshift(uint64_t seed)
        : m_state(seed)
    {
    }

    uint64_t operator()()
    {
        uint64_t x = m_state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return m_state = x;
    }

    uint64_t min() const { return std::numeric_limits<uint64_t>::min(); }
    uint64_t max() const { return std::numeric_limits<uint64_t>::max(); }

private:
    uint64_t m_state;
};

template<std::floating_point T>
class FastNormalDistribution {
public:
    FastNormalDistribution(T mean, T stddev)
        : m_mean(mean)
        , m_stddev(stddev)
    {
    }

    template<class Urng>
    T operator()(Urng& rng) const
    {
        T value = 0;
        constexpr T RangeBase = 0.3649318622825734;
        constexpr size_t Steps = 5;
        T range = m_stddev / RangeBase;
        for (size_t s = 0; s < Steps; s++) {
            T random = static_cast<T>(rng() - rng.min()) / (rng.max() - rng.min()); // 0..1
            value += (random - 0.5) * range;
        }
        return value / Steps + m_mean;
    }

private:
    T m_mean;
    T m_stddev;
};

}
