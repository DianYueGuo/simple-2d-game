#include "eater_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <cmath>

#include <cstdlib>

EaterCircle::EaterCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    EatableCircle(worldId, position_x, position_y, radius, density, friction),
    brain(0, 7) {
    initialize_brain();
    brain.update();
    update_color_from_brain();
}

float calculate_overlap_area(float r1, float r2, float distance) {
    if (distance >= r1 + r2) return 0.0f;
    if (distance <= fabs(r1 - r2)) return 3.14159f * fmin(r1, r2) * fmin(r1, r2);

    float r_sq1 = r1 * r1;
    float r_sq2 = r2 * r2;
    float d_sq = distance * distance;

    float clamp1 = std::clamp((d_sq + r_sq1 - r_sq2) / (2.0f * distance * r1), -1.0f, 1.0f);
    float clamp2 = std::clamp((d_sq + r_sq2 - r_sq1) / (2.0f * distance * r2), -1.0f, 1.0f);

    float part1 = r_sq1 * acos(clamp1);
    float part2 = r_sq2 * acos(clamp2);
    float part3 = 0.5f * sqrt((r1 + r2 - distance) * (r1 - r2 + distance) * (-r1 + r2 + distance) * (r1 + r2 + distance));

    return part1 + part2 - part3;
}

void EaterCircle::process_eating(const b2WorldId &worldId, float poison_death_probability) {
    poisoned = false;
    for (auto* touching_circle : touching_circles) {
        if (touching_circle->getRadius() < this->getRadius()) {
            float touching_area = 3.14159f * touching_circle->getRadius() * touching_circle->getRadius();
            float overlap_threshold = touching_area * 0.8f;

            float distance = b2Distance(this->getPosition(), touching_circle->getPosition());
            float overlap_area = calculate_overlap_area(this->getRadius(), touching_circle->getRadius(), distance);

            if (overlap_area >= overlap_threshold) {
                if (auto* eatable = dynamic_cast<EatableCircle*>(touching_circle)) {
                    if (eatable->is_toxic()) {
                        float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                        if (roll < poison_death_probability) {
                            poisoned = true;
                        }
                        eatable->be_eaten();
                    } else {
                        eatable->be_eaten();
                    }
                }

                float new_area = 3.14159f * this->getRadius() * this->getRadius() + touching_area;
                float new_radius = sqrt(new_area / 3.14159f);
                this->setRadius(new_radius, worldId);
            }
        }
    }

    if (poisoned) {
        this->be_eaten();
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

void EaterCircle::move_intelligently(const b2WorldId &worldId, Game &game) {
    brain.update();
    update_color_from_brain();

    if (brain.read_output(0) >= 1.0f) {
        this->boost_forward(worldId, game);
    }
    if (brain.read_output(1) >= 1.0f) {
        this->apply_left_turn_impulse();
    }
    if (brain.read_output(2) >= 1.0f) {
        this->apply_right_turn_impulse();
    }
    if (brain.read_output(3) >= 1.0f) {
        this->divide(worldId, game);
    }
}

void EaterCircle::boost_forward(const b2WorldId &worldId, Game& game) {
    float current_area = 3.14159f * this->getRadius() * this->getRadius();
    float boost_cost = std::max(game.get_boost_area(), 0.0f);
    float new_area = current_area - boost_cost;

    if (new_area > minimum_area) {
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
        const auto color = get_color_rgb();
        boost_circle_ptr->set_color_rgb(color[0], color[1], color[2]);
        game.add_circle(std::move(boost_circle));
        if (boost_circle_ptr) {
            boost_circle_ptr->setAngle(angle + 3.14159f, worldId);
            boost_circle_ptr->apply_forward_impulse();
        }
    }
}

void EaterCircle::initialize_brain() {
    // Mutate repeatedly to seed a non-trivial brain topology.
    constexpr int mutation_rounds = 60;
    for (int i = 0; i < mutation_rounds; ++i) {
        brain.mutate(0.8f, 0.0f, 1.0f, 0.0f);
    }
}

void EaterCircle::divide(const b2WorldId &worldId, Game& game) {
    const float current_area = 3.14159f * this->getRadius() * this->getRadius();
    const float divided_area = current_area / 2.0f;

    if (divided_area <= minimum_area) {
        return;
    }

    const float new_radius = std::sqrt(divided_area / 3.14159f);

    EaterBrain parent_brain_copy = brain;

    const b2Vec2 original_pos = this->getPosition();

    this->setRadius(new_radius, worldId);

    float angle = this->getAngle();
    b2Vec2 direction = {std::cos(angle), std::sin(angle)};
    b2Vec2 forward_offset = {direction.x * new_radius, direction.y * new_radius};

    b2Vec2 parent_position = {original_pos.x + forward_offset.x, original_pos.y + forward_offset.y};
    b2Vec2 child_position = {original_pos.x - forward_offset.x, original_pos.y - forward_offset.y};

    this->setPosition(parent_position, worldId);

    auto new_circle = std::make_unique<EaterCircle>(worldId, child_position.x, child_position.y, new_radius, 1.0f, 0.3f);
    EaterCircle* new_circle_ptr = new_circle.get();
    if (new_circle_ptr) {
        new_circle_ptr->brain = parent_brain_copy;
        new_circle_ptr->setAngle(angle + 3.14159f, worldId);
        new_circle_ptr->apply_forward_impulse();
        new_circle_ptr->update_color_from_brain();
    }

    this->apply_forward_impulse();

    constexpr int mutation_rounds = 3;
    for (int i = 0; i < mutation_rounds; ++i) {
        brain.mutate(0.6f, 0.2f, 0.5f, 0.5f);
        if (new_circle_ptr) {
            new_circle_ptr->brain.mutate(0.6f, 0.2f, 0.5f, 0.5f);
        }
    }

    update_color_from_brain();
    game.add_circle(std::move(new_circle));
}

void EaterCircle::update_color_from_brain() {
    float r = std::clamp(brain.read_output_input_register(4), 0.0f, 1.0f);
    float g = std::clamp(brain.read_output_input_register(5), 0.0f, 1.0f);
    float b = std::clamp(brain.read_output_input_register(6), 0.0f, 1.0f);
    set_color_rgb(r, g, b);
}
