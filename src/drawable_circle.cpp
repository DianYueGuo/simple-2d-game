#include "drawable_circle.hpp"


DrawableCircle::DrawableCircle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    CirclePhysics(worldId, position_x, position_y, radius, density, friction) {
}

void DrawableCircle::draw(sf::RenderWindow& window) const {
    const CirclePhysics& circle_physics = static_cast<const CirclePhysics&>(*this);

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
