#pragma once

#include <SFML/Graphics.hpp>

class BlendedBackground;

// A background sprite that is compatible with AnimatableProperty.
class AnimatableBackground
{
public:
    explicit AnimatableBackground(sf::Texture const* texture = nullptr)
    : m_texture(std::move(texture)) {}

    AnimatableBackground operator*(double value) const;
    BlendedBackground operator+(AnimatableBackground const&) const;

    sf::Texture const* texture() const { return m_texture; }
    double factor() const { return m_factor; }

private:
    sf::Texture const* m_texture {};
    double m_factor = 1;
};

// The result of adding two AnimatableBackgrounds that is drawn as two
// AnimatableBackgrouns blended into each other.
// Currently, it only supports opacity. More transition modes will
// be added in the future.
class BlendedBackground : public sf::Drawable
{
public:
    BlendedBackground(AnimatableBackground old_image, AnimatableBackground new_image)
    : m_old_image(std::move(old_image)), m_new_image(std::move(new_image)) {}

    virtual void draw(sf::RenderTarget&, sf::RenderStates) const override;

private:
    AnimatableBackground m_old_image;
    AnimatableBackground m_new_image;
};
