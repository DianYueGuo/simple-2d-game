#include "eater_circle.hpp"


EaterCircle::EaterCircle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    EatableCircle(worldId, position_x, position_y, radius, density, friction) {
}

float calculate_overlap_area(float r1, float r2, float distance) {
    if (distance >= r1 + r2) return 0.0f;
    if (distance <= fabs(r1 - r2)) return 3.14159f * fmin(r1, r2) * fmin(r1, r2);

    float r_sq1 = r1 * r1;
    float r_sq2 = r2 * r2;
    float d_sq = distance * distance;

    float part1 = r_sq1 * acos((d_sq + r_sq1 - r_sq2) / (2.0f * distance * r1));
    float part2 = r_sq2 * acos((d_sq + r_sq2 - r_sq1) / (2.0f * distance * r2));
    float part3 = 0.5f * sqrt((r1 + r2 - distance) * (r1 - r2 + distance) * (-r1 + r2 + distance) * (r1 + r2 + distance));

    return part1 + part2 - part3;
}

void EaterCircle::process_eating(const b2WorldId &worldId) {
    for (auto* touching_circle : touching_circles) {
        if (touching_circle->getRadius() < this->getRadius()) {
            float touching_area = 3.14159f * touching_circle->getRadius() * touching_circle->getRadius();
            float overlap_threshold = touching_area * 0.8f;

            float distance = b2Distance(this->getPosition(), touching_circle->getPosition());
            float overlap_area = calculate_overlap_area(this->getRadius(), touching_circle->getRadius(), distance);

            if (overlap_area >= overlap_threshold) {
                float new_area = 3.14159f * this->getRadius() * this->getRadius() + touching_area;
                float new_radius = sqrt(new_area / 3.14159f);
                this->setRadius(new_radius, worldId);

                static_cast<EatableCircle*>(touching_circle)->be_eaten();
            }
        }
    }
}

void EaterCircle::move_randomly(const b2WorldId &worldId, Game &game) {
    float probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->boost_forward(worldId, game);

    probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->apply_left_turn_impulse();

    probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->apply_right_turn_impulse();
}

void EaterCircle::boost_forward(const b2WorldId &worldId, Game& game) {
    float current_area = 3.14159f * this->getRadius() * this->getRadius();
    float boost_cost = 0.3f;
    float new_area = current_area - boost_cost;

    if (new_area > 1.0f) {
        float new_radius = sqrt(new_area / 3.14159f);
        this->setRadius(new_radius, worldId);
        this->apply_forward_impulse();

        float boost_radius = sqrt(boost_cost / 3.14159f);
        b2Vec2 pos = this->getPosition();
        float angle = this->getAngle();
        b2Vec2 direction = {cos(angle), sin(angle)};
        b2Vec2 back_position = {pos.x - direction.x * (this->getRadius() + boost_radius), 
                    pos.y - direction.y * (this->getRadius() + boost_radius)};

        auto boost_circle = std::make_unique<EatableCircle>(worldId, back_position.x, back_position.y, boost_radius, 1.0f, 0.3f);
        EatableCircle* boost_circle_ptr = boost_circle.get();
        game.add_circle(std::move(boost_circle));
        if (boost_circle_ptr) {
            boost_circle_ptr->setAngle(angle + 3.14159f, worldId);
            boost_circle_ptr->apply_forward_impulse();
        }
    }
}
