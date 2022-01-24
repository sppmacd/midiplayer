#pragma once

#include <SFML/Graphics.hpp>

class RoundedEdgeRectangleShape : public sf::Shape
{
public:
    explicit RoundedEdgeRectangleShape(sf::Vector2f size, float borderRadius)
    : m_size(size), m_borderRadius(borderRadius) { update(); }

    virtual std::size_t getPointCount() const override;
    virtual sf::Vector2f getPoint(std::size_t index) const override;

private:
    sf::Vector2f m_size;
    float m_borderRadius {};
};

