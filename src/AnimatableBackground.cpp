#include "AnimatableBackground.h"

#include <iostream>

AnimatableBackground AnimatableBackground::operator*(double value) const
{
    AnimatableBackground tmp { m_texture };
    tmp.m_factor = m_factor * value;
    return tmp;
}

BlendedBackground AnimatableBackground::operator+(AnimatableBackground const& other) const
{
    return BlendedBackground(*this, other);
}

void BlendedBackground::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    // TODO: More transition modes
    if (m_old_image.texture()) {
        sf::Sprite old_bg_sprite { *m_old_image.texture() };
        old_bg_sprite.setColor({ 255, 255, 255, static_cast<std::uint8_t>(255 * m_old_image.factor()) });
        target.setView(sf::View { sf::FloatRect { {}, sf::Vector2f { m_old_image.texture()->getSize() } } });
        target.draw(old_bg_sprite, states);
    }

    if (m_new_image.texture()) {
        sf::Sprite new_bg_sprite { *m_new_image.texture() };
        new_bg_sprite.setColor({ 255, 255, 255, static_cast<std::uint8_t>(255 * m_new_image.factor()) });
        target.setView(sf::View { sf::FloatRect { {}, sf::Vector2f { m_new_image.texture()->getSize() } } });
        target.draw(new_bg_sprite, states);
    }
}
