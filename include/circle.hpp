#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "circle_physics.hpp"

#include <SFML/Graphics.hpp>


class Circle {
public:
    Circle(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    void draw(sf::RenderWindow& window) const;

    CirclePhysics& getPhysics();
private:
    CirclePhysics circle_physics;
};

#endif
