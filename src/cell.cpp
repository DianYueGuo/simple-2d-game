#include "cell.hpp"


Cell::Cell(b2WorldId worldId) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){10.0f, 50.0f};

    bodyId = b2CreateBody(worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 10.0f;
    shapeDef.material.friction = 0.7f;

    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = 10.0f;
    shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
}

Cell::~Cell() {
    b2DestroyShape(shapeId, false);
    b2DestroyBody(bodyId);
}

void Cell::apply_impulse() {
    b2Body_ApplyLinearImpulse(
        bodyId,
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        true
    );
}

void Cell::draw(sf::RenderWindow& window) const {
    b2Circle circle = b2Shape_GetCircle(shapeId);
    sf::CircleShape shape(circle.radius);

    shape.setFillColor(sf::Color(100, 250, 50));

    b2Vec2 position = b2Body_GetPosition(bodyId);
    shape.setPosition({position.x, position.y});

    window.draw(shape);
}
