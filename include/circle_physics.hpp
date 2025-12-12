#ifndef CIRCLE_PHYSICS_HPP
#define CIRCLE_PHYSICS_HPP

#include <functional>
#include <unordered_set>

#include <box2d/box2d.h>

enum class CircleKind {
    Unknown,
    Creature,
    Pellet,
    ToxicPellet,
    DivisionPellet,
    BoostParticle
};

class CirclePhysics {
public:
    struct Config {
        b2Vec2 position;
        float radius;
        float density;
        float angle;
        CircleKind kind;

        Config()
            : position{0.0f, 0.0f},
              radius(1.0f),
              density(1.0f),
              angle(0.0f),
              kind(CircleKind::Unknown) {}

        Config(b2Vec2 position_, float radius_, float density_, float angle_, CircleKind kind_)
            : position(position_),
              radius(radius_),
              density(density_),
              angle(angle_),
              kind(kind_) {}
    };

    explicit CirclePhysics(const b2WorldId &worldId, Config config = {});

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
    void grow_by_area(float delta_area, const b2WorldId& worldId);

    void apply_forward_force() const;
    void apply_zero_force() const;
    void apply_left_turn_torque() const;
    void apply_right_turn_torque() const;
    void apply_zero_torque() const;

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
    CircleKind get_kind() const { return kind; }
    void for_each_touching(const std::function<void(CirclePhysics&)>& fn);
    void for_each_touching(const std::function<void(const CirclePhysics&)>& fn) const;
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
    CircleKind kind;
protected:
    std::unordered_set<CirclePhysics*> touching_circles;
    void set_kind(CircleKind k) { kind = k; }
};

#endif
