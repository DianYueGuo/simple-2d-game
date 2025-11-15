#include "cell.hpp"


Cell::Cell(b2WorldId worldId) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){0.0f, 0.0f};

    bodyId = b2CreateBody(worldId, &bodyDef);
}

Cell::~Cell() {
    b2DestroyBody(bodyId);
}

void Cell::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    window.draw(shape);
}
