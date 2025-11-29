#include "circle.hpp"


Circle::Circle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    circle_physics(worldId, position_x, position_y, radius, density, friction) {
}

void Circle::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    shape.setOrigin({circle_physics.getRadius(), circle_physics.getRadius()});
    shape.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});
    shape.setRadius(circle_physics.getRadius());
    window.draw(shape);

    sf::RectangleShape line({circle_physics.getRadius(), circle_physics.getRadius() / 4.0f});
    line.setFillColor(sf::Color::White);
    line.rotate(sf::radians(circle_physics.getAngle()));

    line.setOrigin({0, circle_physics.getRadius() / 4.0f / 2.0f});
    line.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});

    window.draw(line);
}

CirclePhysics& Circle::getPhysics() {
    return circle_physics;
}
