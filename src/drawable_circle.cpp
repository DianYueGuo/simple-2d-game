#include "drawable_circle.hpp"


DrawableCircle::DrawableCircle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    CirclePhysics(worldId, position_x, position_y, radius, density, friction) {
}

void DrawableCircle::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    shape.setOrigin({getRadius(), getRadius()});
    shape.setPosition({getPosition().x, getPosition().y});
    shape.setRadius(getRadius());
    window.draw(shape);

    sf::RectangleShape line({getRadius(), getRadius() / 4.0f});
    line.setFillColor(sf::Color::White);
    line.rotate(sf::radians(getAngle()));

    line.setOrigin({0, getRadius() / 4.0f / 2.0f});
    line.setPosition({getPosition().x, getPosition().y});

    window.draw(line);
}
