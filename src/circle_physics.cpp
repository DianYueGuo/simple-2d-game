#include "circle_physics.hpp"


CirclePhysics::CirclePhysics(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    bodyId{} {
    b2BodyDef circleBodyDef = b2DefaultBodyDef();
    circleBodyDef.type = b2_dynamicBody;
    circleBodyDef.position = (b2Vec2){position_x, position_y};

    circleBodyDef.linearDamping = 0.3f;
    circleBodyDef.angularDamping = 1.0f;

    bodyId = b2CreateBody(worldId, &circleBodyDef);

    b2ShapeDef CircleShapeDef = b2DefaultShapeDef();
    CircleShapeDef.density = density;
    CircleShapeDef.material.friction = friction;

    CircleShapeDef.userData = this;

    CircleShapeDef.isSensor = true;
    CircleShapeDef.enableSensorEvents = true;

    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = radius;

    b2CreateCircleShape(bodyId, &CircleShapeDef, &circle);
}

CirclePhysics::~CirclePhysics() {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }

    for (auto* touching_circle : touching_circles) {
        touching_circle->remove_touching_circle(this);
    }
}

CirclePhysics::CirclePhysics(CirclePhysics&& other_circle_physics) noexcept :
    bodyId(other_circle_physics.bodyId),
    touching_circles(std::move(other_circle_physics.touching_circles)) {

    b2ShapeId shapeId;
    b2Body_GetShapes(bodyId, &shapeId, 1);
    b2Shape_SetUserData(shapeId, this);

    other_circle_physics.bodyId = (b2BodyId){};

    for (auto* touching_circle : touching_circles) {
        touching_circle->remove_touching_circle(&other_circle_physics);
        touching_circle->add_touching_circle(this);
    }
}

CirclePhysics& CirclePhysics::operator=(CirclePhysics&& other_circle_physics) noexcept {
    if (this == &other_circle_physics) {
        return *this;
    }

    if (b2Body_IsValid(bodyId)) b2DestroyBody(bodyId);
    bodyId = other_circle_physics.bodyId;

    b2ShapeId shapeId;
    b2Body_GetShapes(bodyId, &shapeId, 1);
    b2Shape_SetUserData(shapeId, this);

        other_circle_physics.bodyId = (b2BodyId){};

    touching_circles = std::move(other_circle_physics.touching_circles);
    for (auto* touching_circle : touching_circles) {
        touching_circle->remove_touching_circle(&other_circle_physics);
        touching_circle->add_touching_circle(this);
    }

    return *this;
}

b2Vec2 CirclePhysics::getPosition() const {
    return b2Body_GetPosition(bodyId);
}

float CirclePhysics::getRadius() const {
    b2ShapeId shapeId;
    b2Body_GetShapes(bodyId, &shapeId, 1);
    b2Circle circle = b2Shape_GetCircle(shapeId);
    return circle.radius;
}

void CirclePhysics::apply_forward_force() const {
    b2Rot rotation = b2Body_GetRotation(bodyId);
    float force_magnitude = 10000.0f;
    b2Vec2 force = {force_magnitude * rotation.c, force_magnitude * rotation.s};
    b2Body_ApplyForceToCenter(bodyId, force, true);
};

void CirclePhysics::stop_applying_force() const {
    b2Vec2 force = {0.0f, 0.0f};
    b2Body_ApplyForceToCenter(bodyId, force, true);
};

void CirclePhysics::apply_left_turn_torque() const {
    b2Body_ApplyTorque(bodyId, -100000.0f, true);
};

void CirclePhysics::apply_right_turn_torque() const {
    b2Body_ApplyTorque(bodyId, 100000.0f, true);
};

void CirclePhysics::stop_applying_torque() const {
    b2Body_ApplyTorque(bodyId, 0.0f, true);
};

float CirclePhysics::getAngle() const {
    return b2Rot_GetAngle(b2Body_GetRotation(bodyId));
}

void CirclePhysics::add_touching_circle(CirclePhysics* circle_physics) {
    touching_circles.insert(circle_physics);
}

void CirclePhysics::remove_touching_circle(CirclePhysics* circle_physics) {
    touching_circles.erase(circle_physics);
}

void CirclePhysics::setRadius(float new_radius, const b2WorldId &worldId) {
    if (!b2Body_IsValid(bodyId)) return;

    // Get current body properties
    b2Vec2 position = b2Body_GetPosition(bodyId);
    b2Rot rotation = b2Body_GetRotation(bodyId);
    b2Vec2 linearVelocity = b2Body_GetLinearVelocity(bodyId);
    float angularVelocity = b2Body_GetAngularVelocity(bodyId);

    // Destroy old body
    b2DestroyBody(bodyId);

    // Create new body with same properties
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = position;
    bodyDef.rotation = rotation;
    bodyDef.linearVelocity = linearVelocity;
    bodyDef.angularVelocity = angularVelocity;
    bodyDef.linearDamping = 0.3f;
    bodyDef.angularDamping = 1.0f;

    bodyId = b2CreateBody(worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.5f;
    shapeDef.userData = this;
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;

    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = new_radius;

    b2CreateCircleShape(bodyId, &shapeDef, &circle);
}
