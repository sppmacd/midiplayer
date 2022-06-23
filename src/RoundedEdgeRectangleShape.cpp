#include "RoundedEdgeRectangleShape.hpp"

#include <cmath>
#include <iostream>

constexpr size_t ROUNDED_EDGE_RESOLUTION = 10;

std::size_t RoundedEdgeRectangleShape::getPointCount() const
{
    return ROUNDED_EDGE_RESOLUTION * 4;
}

sf::Vector2f RoundedEdgeRectangleShape::getPoint(std::size_t index) const
{
    size_t side = index / ROUNDED_EDGE_RESOLUTION;

    float subangle = (float)(index % ROUNDED_EDGE_RESOLUTION) / (ROUNDED_EDGE_RESOLUTION - 1) * M_PI_2;
    float angle = subangle + side * M_PI_2;

    float radius = std::min(std::min(m_borderRadius, m_size.x / 2), m_size.y / 2);

    sf::Vector2f offset { std::cos(angle) * radius, std::sin(angle) * radius };
    sf::Vector2f base;
    switch (side) {
        case 0:
            base = { m_size.x - radius, m_size.y - radius };
            break;
        case 1:
            base = { radius, m_size.y - radius };
            break;
        case 2:
            base = { radius, radius };
            break;
        case 3:
            base = { m_size.x - radius, radius };
            break;
        default:
            break;
    }
    auto result = base + offset;
    return result;
}
