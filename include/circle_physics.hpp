#ifndef CIRCLE_PHYSICS_HPP
#define CIRCLE_PHYSICS_HPP

#include <unordered_set>

#include <box2d/box2d.h>


class CirclePhysics {
public:
    CirclePhysics(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    virtual ~CirclePhysics();

    CirclePhysics(const CirclePhysics&) = delete;
    CirclePhysics& operator=(const CirclePhysics&) = delete;

    CirclePhysics(CirclePhysics&& other_circle_physics) noexcept;

    CirclePhysics& operator=(CirclePhysics&& other_circle_physics) noexcept;

    b2Vec2 getPosition() const;

    float getRadius() const;

    void apply_forward_force() const;
    void stop_applying_force() const;
    void apply_left_turn_torque() const;
    void apply_right_turn_torque() const;
    void stop_applying_torque() const;

    void apply_forward_impulse() const;
    void apply_left_turn_impulse() const;
    void apply_right_turn_impulse() const;

    float getAngle() const;

    void add_touching_circle(CirclePhysics* circle_physics);
    void remove_touching_circle(CirclePhysics* circle_physics);

    void setRadius(float new_radius, const b2WorldId &worldId);
    void setAngle(float new_angle, const b2WorldId &worldId);
private:
    b2BodyId bodyId;
protected:
    std::unordered_set<CirclePhysics*> touching_circles;
};

#endif
