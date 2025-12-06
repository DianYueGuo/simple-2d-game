#include "eater_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <cmath>

#include <cstdlib>

namespace {
float neat_activation(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}
} // namespace

EaterCircle::EaterCircle(const b2WorldId &worldId,
                         float position_x,
                         float position_y,
                         float radius,
                         float density,
                         float angle,
                         int generation,
                         int init_mutation_rounds,
                         float init_add_node_probability,
                         float init_remove_node_probability,
                         float init_add_connection_probability,
                         float init_remove_connection_probability,
                         const neat::Genome* base_brain,
                         std::vector<std::vector<int>>* innov_ids,
                         int* last_innov_id) :
    EatableCircle(worldId, position_x, position_y, radius, density, false, angle),
    brain(base_brain ? *base_brain : neat::Genome(29, 7, 0, 0.5f, innov_ids, last_innov_id)) {
    neat_innovations = innov_ids;
    neat_last_innov_id = last_innov_id;
    set_generation(generation);
    initialize_brain(
        init_mutation_rounds,
        init_add_node_probability,
        init_remove_node_probability,
        init_add_connection_probability,
        init_remove_connection_probability);
    update_brain_inputs_from_touching();
    brain.loadInputs(brain_inputs.data());
    brain.runNetwork(neat_activation);
    brain.getOutputs(brain_outputs.data());
    update_color_from_brain();
    smooth_display_color(1.0f); // start display at brain-driven color immediately
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

void EaterCircle::process_eating(const b2WorldId &worldId, float poison_death_probability_toxic, float poison_death_probability_normal) {
    poisoned = false;
    for (auto* touching_circle : touching_circles) {
        if (touching_circle->getRadius() < this->getRadius()) {
            float touching_area = touching_circle->getArea();
            float overlap_threshold = touching_area * 0.8f;

            float distance = b2Distance(this->getPosition(), touching_circle->getPosition());
            float overlap_area = calculate_overlap_area(this->getRadius(), touching_circle->getRadius(), distance);

            if (overlap_area >= overlap_threshold) {
                if (auto* eatable = dynamic_cast<EatableCircle*>(touching_circle)) {
                    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                    if (eatable->is_toxic()) {
                        if (roll < poison_death_probability_toxic) {
                            poisoned = true;
                        }
                        eatable->be_eaten();
                    } else {
                        if (roll < poison_death_probability_normal) {
                            poisoned = true;
                        }
                        eatable->be_eaten();
                    }
                }

                float new_area = this->getArea() + touching_area;
                this->setArea(new_area, worldId);
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

void EaterCircle::move_intelligently(const b2WorldId &worldId, Game &game, float dt) {
    update_brain_inputs_from_touching();
    brain.loadInputs(brain_inputs.data());
    brain.runNetwork(neat_activation);
    brain.getOutputs(brain_outputs.data());
    update_color_from_brain();

    if (brain_outputs[0] >= 0.5f) {
        this->boost_forward(worldId, game);
    }
    if (brain_outputs[1] >= 0.5f) {
        this->apply_left_turn_impulse();
    }
    if (brain_outputs[2] >= 0.5f) {
        this->apply_right_turn_impulse();
    }
    if (brain_outputs[3] >= 0.5f) {
        this->divide(worldId, game);
    }

    // Inactivity based on zero linear velocity.
    inactivity_timer += std::max(0.0f, dt);
    b2Vec2 velocity = getLinearVelocity();
    constexpr float vel_epsilon = 1e-3f;
    if (std::fabs(velocity.x) > vel_epsilon || std::fabs(velocity.y) > vel_epsilon) {
        inactivity_timer = 0.0f;
    } else {
        float timeout = game.get_inactivity_timeout();
        if (timeout > 0.0f && inactivity_timer >= timeout && !is_eaten()) {
            poisoned = true;
            this->be_eaten();
            inactivity_timer = 0.0f;
        }
    }

    if (neat_innovations && neat_last_innov_id) {
        brain.mutate(
            neat_innovations,
            neat_last_innov_id,
            false,
            0.8f,
            0.1f,
            1.2f,
            game.get_tick_add_connection_probability(),
            20,
            0.25f,
            game.get_tick_add_node_probability(),
            20);
    }

    // Update memory from the first four outputs (clamped).
    for (int i = 0; i < 4; ++i) {
        memory_state[i] = std::clamp(brain_outputs[i], 0.0f, 1.0f);
    }
}

void EaterCircle::boost_forward(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(game.get_boost_area(), 0.0f);
    float new_area = current_area - boost_cost;

    if (boost_cost <= 0.0f) {
        // No cost, just apply impulse without leaving a circle.
        this->apply_forward_impulse();
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        this->apply_forward_impulse();

        float boost_radius = sqrt(boost_cost / 3.14159f);
        b2Vec2 pos = this->getPosition();
        float angle = this->getAngle();
        b2Vec2 direction = {cos(angle), sin(angle)};
        b2Vec2 back_position = {pos.x - direction.x * (this->getRadius() + boost_radius), 
                    pos.y - direction.y * (this->getRadius() + boost_radius)};

        auto boost_circle = std::make_unique<EatableCircle>(worldId, back_position.x, back_position.y, boost_radius, game.get_circle_density(), false, 0.0f);
        EatableCircle* boost_circle_ptr = boost_circle.get();
        const auto eater_signal_color = get_color_rgb(); // use true signal, not smoothed display
        boost_circle_ptr->set_color_rgb(eater_signal_color[0], eater_signal_color[1], eater_signal_color[2]);
        boost_circle_ptr->smooth_display_color(1.0f);
        boost_circle_ptr->set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
        boost_circle_ptr->set_linear_damping(game.get_linear_damping(), worldId);
        boost_circle_ptr->set_angular_damping(game.get_angular_damping(), worldId);
        game.add_circle(std::move(boost_circle));
        if (boost_circle_ptr) {
            boost_circle_ptr->setAngle(angle + 3.14159f, worldId);
            boost_circle_ptr->apply_forward_impulse();
        }
    }
}

void EaterCircle::initialize_brain(int mutation_rounds, float add_node_p, float remove_node_p, float add_connection_p, float remove_connection_p) {
    // Mutate repeatedly to seed a non-trivial brain topology.
    int rounds = std::max(0, mutation_rounds);
    for (int i = 0; i < rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                false,
                0.8f,
                0.1f,
                1.2f,
                add_connection_p,
                20,
                0.25f,
                add_node_p,
                20);
        }
    }
}

void EaterCircle::divide(const b2WorldId &worldId, Game& game) {
    const float current_area = this->getArea();
    const float divided_area = current_area / 2.0f;

    if (divided_area <= minimum_area) {
        return;
    }

    const float new_radius = std::sqrt(divided_area / 3.14159f);

    neat::Genome parent_brain_copy = brain;

    const b2Vec2 original_pos = this->getPosition();

    this->setRadius(new_radius, worldId);

    float angle = this->getAngle();
    b2Vec2 direction = {std::cos(angle), std::sin(angle)};
    b2Vec2 forward_offset = {direction.x * new_radius, direction.y * new_radius};

    b2Vec2 parent_position = {original_pos.x + forward_offset.x, original_pos.y + forward_offset.y};
    b2Vec2 child_position = {original_pos.x - forward_offset.x, original_pos.y - forward_offset.y};

    this->setPosition(parent_position, worldId);

    const int next_generation = this->get_generation() + 1;

    auto new_circle = std::make_unique<EaterCircle>(
        worldId,
        child_position.x,
        child_position.y,
        new_radius,
        game.get_circle_density(),
        angle + 3.14159f,
        next_generation,
        game.get_init_mutation_rounds(),
        game.get_init_add_node_probability(),
        game.get_init_remove_node_probability(),
        game.get_init_add_connection_probability(),
        game.get_init_remove_connection_probability(),
        &brain,
        game.get_neat_innovations(),
        game.get_neat_last_innovation_id());
    EaterCircle* new_circle_ptr = new_circle.get();
    if (new_circle_ptr) {
        new_circle_ptr->brain = parent_brain_copy;
        new_circle_ptr->set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
        new_circle_ptr->set_linear_damping(game.get_linear_damping(), worldId);
        new_circle_ptr->set_angular_damping(game.get_angular_damping(), worldId);
        new_circle_ptr->setAngle(angle + 3.14159f, worldId);
        new_circle_ptr->apply_forward_impulse();
        new_circle_ptr->update_color_from_brain();
    }

    this->set_generation(next_generation);
    if (new_circle_ptr) {
        new_circle_ptr->set_generation(next_generation);
    }

    game.update_max_generation_from_circle(this);
    game.update_max_generation_from_circle(new_circle_ptr);

    this->apply_forward_impulse();

    const int mutation_rounds = std::max(0, game.get_mutation_rounds());
    for (int i = 0; i < mutation_rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                false,
                0.8f,
                0.1f,
                1.2f,
                game.get_add_connection_probability(),
                20,
                0.25f,
                game.get_add_node_probability(),
                20);
        }
        if (new_circle_ptr && new_circle_ptr->neat_innovations && new_circle_ptr->neat_last_innov_id) {
            new_circle_ptr->brain.mutate(
                new_circle_ptr->neat_innovations,
                new_circle_ptr->neat_last_innov_id,
                false,
                0.8f,
                0.1f,
                1.2f,
                game.get_add_connection_probability(),
                20,
                0.25f,
                game.get_add_node_probability(),
                20);
        }
    }

    update_color_from_brain();
    game.add_circle(std::move(new_circle));
}

void EaterCircle::update_color_from_brain() {
    float target_r = std::clamp(brain_outputs[4], 0.0f, 1.0f);
    float target_g = std::clamp(brain_outputs[5], 0.0f, 1.0f);
    float target_b = std::clamp(brain_outputs[6], 0.0f, 1.0f);
    set_color_rgb(target_r, target_g, target_b); // keep the signal exact
    constexpr float smoothing = 0.1f; // exponential smoothing factor for display only
    smooth_display_color(smoothing);
}

void EaterCircle::update_brain_inputs_from_touching() {
    constexpr float PI = 3.14159f;
    constexpr int SENSOR_COUNT = 8;
    constexpr float SECTOR_WIDTH = PI / 4.0f; // 45 degrees
    constexpr float SECTOR_HALF = SECTOR_WIDTH * 0.5f;

    std::array<std::array<float, 3>, SENSOR_COUNT> summed_colors{};
    std::array<int, SENSOR_COUNT> counts{};

    if (!touching_circles.empty()) {
        const b2Vec2 self_pos = this->getPosition();
        const float heading = this->getAngle();

        auto normalize_angle = [](float angle) {
            constexpr float TWO_PI = PI * 2.0f;
            angle = std::fmod(angle, TWO_PI);
            if (angle > PI) {
                angle -= TWO_PI;
            } else if (angle < -PI) {
                angle += TWO_PI;
            }
            return angle;
        };

        auto classify_sector = [&](float relative_angle) {
            // Offset by half sector so sector 0 is centered on the heading direction.
            int sector = static_cast<int>(std::floor((relative_angle + SECTOR_HALF) / SECTOR_WIDTH));
            sector = (sector % SENSOR_COUNT + SENSOR_COUNT) % SENSOR_COUNT;
            return sector;
        };

        for (auto* circle : touching_circles) {
            auto* drawable = dynamic_cast<DrawableCircle*>(circle);
            if (!drawable) {
                continue;
            }

            const b2Vec2 other_pos = circle->getPosition();
            const float dx = other_pos.x - self_pos.x;
            const float dy = other_pos.y - self_pos.y;

            if (dx == 0.0f && dy == 0.0f) {
                continue;
            }

            float relative_angle = std::atan2(dy, dx) - heading;
            relative_angle = normalize_angle(relative_angle);

            int sector = classify_sector(relative_angle);
            const auto color = drawable->get_color_rgb();
            summed_colors[sector][0] += color[0];
            summed_colors[sector][1] += color[1];
            summed_colors[sector][2] += color[2];
            ++counts[sector];
        }
    }

    auto set_inputs_for_sector = [&](int base_index, const std::array<float, 3>& color_sum, int count) {
        if (count > 0) {
            float inv = 1.0f / static_cast<float>(count);
            brain_inputs[base_index]     = color_sum[0] * inv;
            brain_inputs[base_index + 1] = color_sum[1] * inv;
            brain_inputs[base_index + 2] = color_sum[2] * inv;
        } else {
            brain_inputs[base_index]     = 0.0f;
            brain_inputs[base_index + 1] = 0.0f;
            brain_inputs[base_index + 2] = 0.0f;
        }
    };

    // Order sensors clockwise starting from front: front, front-right, right, back-right, back, back-left, left, front-left.
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        set_inputs_for_sector(i * 3, summed_colors[i], counts[i]);
    }

    // Size input at the end: normalized area to [0,1]
    float area = this->getArea();
    float normalized = area / (area + 1.0f); // saturating transform
    brain_inputs[24] = normalized;

    // Memory inputs (indices 25-28)
    for (int i = 0; i < 4; ++i) {
        brain_inputs[25 + i] = memory_state[i];
    }
}
