#ifndef CIRCLE_PHYSICS_HPP
#define CIRCLE_PHYSICS_HPP

#include <unordered_set>

#include <box2d/box2d.h>


class CirclePhysics {
public:
    CirclePhysics(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float angle = 0.0f);

    virtual ~CirclePhysics();

    CirclePhysics(const CirclePhysics&) = delete;
    CirclePhysics& operator=(const CirclePhysics&) = delete;

    CirclePhysics(CirclePhysics&& other_circle_physics) noexcept;

    CirclePhysics& operator=(CirclePhysics&& other_circle_physics) noexcept;

    b2Vec2 getPosition() const;
    b2Vec2 getLinearVelocity() const;

    float getRadius() const;
    float getArea() const;
    void setArea(float area, const b2WorldId& worldId);

    void apply_forward_force() const;
    void stop_applying_force() const;
    void apply_left_turn_torque() const;
    void apply_right_turn_torque() const;
    void stop_applying_torque() const;

    void apply_forward_impulse() const;
    void apply_left_turn_impulse() const;
    void apply_right_turn_impulse() const;

    float getAngle() const;

    void set_density(float new_density, const b2WorldId& worldId);
    void set_impulse_magnitudes(float linear, float angular);
    void set_linear_damping(float damping, const b2WorldId& worldId);
    void set_angular_damping(float damping, const b2WorldId& worldId);

    void add_touching_circle(CirclePhysics* circle_physics);
    void remove_touching_circle(CirclePhysics* circle_physics);

    void setRadius(float new_radius, const b2WorldId &worldId);
    void setPosition(const b2Vec2& new_position, const b2WorldId &worldId);
    void setAngle(float new_angle, const b2WorldId &worldId);
private:
    struct BodyState {
        b2Vec2 position;
        b2Rot rotation;
        b2Vec2 linearVelocity;
        float angularVelocity;
        float radius;
    };

    BodyState captureBodyState() const;
    b2BodyDef buildBodyDef(const BodyState& state) const;
    b2ShapeDef buildCircleShapeDef() const;
    void createBodyWithState(const b2WorldId& worldId, const BodyState& state);
    void recreateBodyWithState(const b2WorldId& worldId, const BodyState& state);

    b2BodyId bodyId;
    float density;
    bool isSensor;
    bool enableSensorEvents;
    float linearDamping;
    float angularDamping;
    float linearImpulseMagnitude;
    float angularImpulseMagnitude;
protected:
    std::unordered_set<CirclePhysics*> touching_circles;
};

#endif
