#include "drawable_circle.hpp"


DrawableCircle::DrawableCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    CirclePhysics(worldId, position_x, position_y, radius, density, friction) {
}

void DrawableCircle::draw(sf::RenderWindow& window, float pixle_per_meter) const {
    sf::CircleShape shape(getRadius() * pixle_per_meter);
    shape.setFillColor(sf::Color::Green);

    shape.setOrigin({getRadius() * pixle_per_meter, getRadius() * pixle_per_meter});
    shape.setPosition({getPosition().x * pixle_per_meter, getPosition().y * pixle_per_meter});
    window.draw(shape);

    sf::RectangleShape line({getRadius() * pixle_per_meter, getRadius() * pixle_per_meter / 4.0f});
    line.setFillColor(sf::Color::White);
    line.rotate(sf::radians(getAngle()));

    line.setOrigin({0, getRadius() * pixle_per_meter / 4.0f / 2.0f});
    line.setPosition({getPosition().x * pixle_per_meter, getPosition().y * pixle_per_meter});

    window.draw(line);
}
