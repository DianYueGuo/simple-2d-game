#include "circle_physics.hpp"


CirclePhysics::CirclePhysics(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    bodyId{},
    density(density),
    friction(friction),
    isSensor(true),
    enableSensorEvents(true),
    linearDamping(0.3f),
    angularDamping(1.0f) {
    BodyState initialState{};
    initialState.position = (b2Vec2){position_x, position_y};
    initialState.rotation = b2MakeRot(0.0f);
    initialState.linearVelocity = (b2Vec2){0.0f, 0.0f};
    initialState.angularVelocity = 0.0f;
    initialState.radius = radius;

    createBodyWithState(worldId, initialState);
}

CirclePhysics::~CirclePhysics() {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }

    for (auto* touching_circle : touching_circles) {
        touching_circle->remove_touching_circle(this);
    }
}

CirclePhysics::BodyState CirclePhysics::captureBodyState() const {
    BodyState state{};
    state.position = b2Body_GetPosition(bodyId);
    state.rotation = b2Body_GetRotation(bodyId);
    state.linearVelocity = b2Body_GetLinearVelocity(bodyId);
    state.angularVelocity = b2Body_GetAngularVelocity(bodyId);
    state.radius = getRadius();
    return state;
}

b2BodyDef CirclePhysics::buildBodyDef(const BodyState& state) const {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = state.position;
    bodyDef.rotation = state.rotation;
    bodyDef.linearVelocity = state.linearVelocity;
    bodyDef.angularVelocity = state.angularVelocity;
    bodyDef.linearDamping = linearDamping;
    bodyDef.angularDamping = angularDamping;
    return bodyDef;
}

b2ShapeDef CirclePhysics::buildCircleShapeDef() const {
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.material.friction = friction;
    shapeDef.userData = const_cast<CirclePhysics*>(this);
    shapeDef.isSensor = isSensor;
    shapeDef.enableSensorEvents = enableSensorEvents;
    return shapeDef;
}

void CirclePhysics::createBodyWithState(const b2WorldId& worldId, const BodyState& state) {
    b2BodyDef bodyDef = buildBodyDef(state);
    bodyId = b2CreateBody(worldId, &bodyDef);

    b2ShapeDef shapeDef = buildCircleShapeDef();
    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = state.radius;

    b2CreateCircleShape(bodyId, &shapeDef, &circle);
}

void CirclePhysics::recreateBodyWithState(const b2WorldId& worldId, const BodyState& state) {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }
    createBodyWithState(worldId, state);
}

CirclePhysics::CirclePhysics(CirclePhysics&& other_circle_physics) noexcept :
    bodyId(other_circle_physics.bodyId),
    density(other_circle_physics.density),
    friction(other_circle_physics.friction),
    isSensor(other_circle_physics.isSensor),
    enableSensorEvents(other_circle_physics.enableSensorEvents),
    linearDamping(other_circle_physics.linearDamping),
    angularDamping(other_circle_physics.angularDamping),
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
    density = other_circle_physics.density;
    friction = other_circle_physics.friction;
    isSensor = other_circle_physics.isSensor;
    enableSensorEvents = other_circle_physics.enableSensorEvents;
    linearDamping = other_circle_physics.linearDamping;
    angularDamping = other_circle_physics.angularDamping;

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

float CirclePhysics::getArea() const {
    float r = getRadius();
    return 3.14159f * r * r;
}

void CirclePhysics::apply_forward_force() const {
    b2Rot rotation = b2Body_GetRotation(bodyId);
    float force_magnitude = 50.0f;
    b2Vec2 force = {force_magnitude * rotation.c, force_magnitude * rotation.s};
    b2Body_ApplyForceToCenter(bodyId, force, true);
};

void CirclePhysics::stop_applying_force() const {
    b2Vec2 force = {0.0f, 0.0f};
    b2Body_ApplyForceToCenter(bodyId, force, true);
};

void CirclePhysics::apply_left_turn_torque() const {
    b2Body_ApplyTorque(bodyId, -50.0f, true);
};

void CirclePhysics::apply_right_turn_torque() const {
    b2Body_ApplyTorque(bodyId, 50.0f, true);
};

void CirclePhysics::stop_applying_torque() const {
    b2Body_ApplyTorque(bodyId, 0.0f, true);
};

void CirclePhysics::apply_forward_impulse() const {
    b2Rot rotation = b2Body_GetRotation(bodyId);
    float impulse_magnitude = 5.0f;
    b2Vec2 impulse = {impulse_magnitude * rotation.c, impulse_magnitude * rotation.s};
    b2Body_ApplyLinearImpulse(bodyId, impulse, b2Body_GetPosition(bodyId), true);
};

void CirclePhysics::apply_left_turn_impulse() const {
    b2Body_ApplyAngularImpulse(bodyId, -5.0f, true);
};

void CirclePhysics::apply_right_turn_impulse() const {
    b2Body_ApplyAngularImpulse(bodyId, 5.0f, true);
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

    BodyState state = captureBodyState();
    state.radius = new_radius;
    recreateBodyWithState(worldId, state);
}

void CirclePhysics::setArea(float area, const b2WorldId &worldId) {
    if (area <= 0.0f) {
        return;
    }
    float new_radius = std::sqrt(area / 3.14159f);
    setRadius(new_radius, worldId);
}

void CirclePhysics::setPosition(const b2Vec2& new_position, const b2WorldId &worldId) {
    if (!b2Body_IsValid(bodyId)) return;

    BodyState state = captureBodyState();
    state.position = new_position;
    recreateBodyWithState(worldId, state);
}

void CirclePhysics::setAngle(float new_angle, const b2WorldId &worldId) {
    if (!b2Body_IsValid(bodyId)) return;

    BodyState state = captureBodyState();
    state.rotation = b2MakeRot(new_angle);
    recreateBodyWithState(worldId, state);
}
