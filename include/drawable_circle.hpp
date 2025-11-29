#ifndef DRAWABLE_CIRCLE_HPP
#define DRAWABLE_CIRCLE_HPP

#include "circle_physics.hpp"

#include <SFML/Graphics.hpp>


class DrawableCircle : public CirclePhysics {
public:
    DrawableCircle(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    void draw(sf::RenderWindow& window) const;
};

#endif
