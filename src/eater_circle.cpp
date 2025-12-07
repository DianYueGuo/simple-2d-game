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
                         int* last_innov_id,
                         Game* owner) :
    EatableCircle(worldId, position_x, position_y, radius, density, false, angle),
    brain(base_brain ? *base_brain : neat::Genome(29, 11, 0, 0.5f, innov_ids, last_innov_id)) {
    neat_innovations = innov_ids;
    neat_last_innov_id = last_innov_id;
    owner_game = owner;
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

void EaterCircle::process_eating(const b2WorldId &worldId, Game& game, float poison_death_probability_toxic, float poison_death_probability_normal) {
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
                        eatable->set_eaten_by(this);
                    } else {
                        if (roll < poison_death_probability_normal) {
                            poisoned = true;
                        }
                        eatable->be_eaten();
                        eatable->set_eaten_by(this);
                        if (eatable->is_division_boost()) {
                            float div_roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                            if (div_roll <= game.get_division_pellet_divide_probability()) {
                                this->divide(worldId, game);
                            }
                        }
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
        if (timeout <= 0.0f) {
            if (!is_eaten()) {
                poisoned = true;
                this->be_eaten();
            }
            inactivity_timer = 0.0f;
        } else if (inactivity_timer >= timeout && !is_eaten()) {
            poisoned = true;
            this->be_eaten();
            inactivity_timer = 0.0f;
        }
    }

    if (game.get_live_mutation_enabled() && neat_innovations && neat_last_innov_id) {
        brain.mutate(
            neat_innovations,
            neat_last_innov_id,
            game.get_mutate_allow_recurrent(),
            game.get_mutate_weight_thresh(),
            game.get_mutate_weight_full_change_thresh(),
            game.get_mutate_weight_factor(),
            game.get_tick_add_connection_probability(),
            game.get_mutate_add_connection_iterations(),
            game.get_mutate_reactivate_connection_thresh(),
            game.get_tick_add_node_probability(),
            game.get_mutate_add_node_iterations());
    }

    // Update memory from the first four outputs (clamped).
    // Memory outputs are distinct: outputs 7-10.
    for (int i = 0; i < 4; ++i) {
        memory_state[i] = std::clamp(brain_outputs[7 + i], 0.0f, 1.0f);
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

        auto boost_circle = std::make_unique<EatableCircle>(
            worldId,
            back_position.x,
            back_position.y,
            boost_radius,
            game.get_circle_density(),
            /*toxic=*/false,
            /*division_boost=*/false,
            /*angle=*/0.0f,
            /*boost_particle=*/true);
        EatableCircle* boost_circle_ptr = boost_circle.get();
        const auto eater_signal_color = get_color_rgb(); // use true signal, not smoothed display
        boost_circle_ptr->set_color_rgb(eater_signal_color[0], eater_signal_color[1], eater_signal_color[2]);
        boost_circle_ptr->smooth_display_color(1.0f);
        float frac = game.get_boost_particle_impulse_fraction();
        boost_circle_ptr->set_impulse_magnitudes(game.get_linear_impulse_magnitude() * frac, game.get_angular_impulse_magnitude() * frac);
        boost_circle_ptr->set_linear_damping(game.get_boost_particle_linear_damping(), worldId);
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
            float weight_thresh = 0.8f;
            float weight_full = 0.1f;
            float weight_factor = 1.2f;
            float reactivate = 0.25f;
            int add_conn_iters = 20;
            int add_node_iters = 20;
            bool allow_recurrent = false;
            if (owner_game) {
                weight_thresh = owner_game->get_mutate_weight_thresh();
                weight_full = owner_game->get_mutate_weight_full_change_thresh();
                weight_factor = owner_game->get_mutate_weight_factor();
                reactivate = owner_game->get_mutate_reactivate_connection_thresh();
                add_conn_iters = owner_game->get_mutate_add_connection_iterations();
                add_node_iters = owner_game->get_mutate_add_node_iterations();
                allow_recurrent = owner_game->get_mutate_allow_recurrent();
            }
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                allow_recurrent,
                weight_thresh,
                weight_full,
                weight_factor,
                add_connection_p,
                add_conn_iters,
                reactivate,
                add_node_p,
                add_node_iters);
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
        game.get_neat_last_innovation_id(),
        &game);
    EaterCircle* new_circle_ptr = new_circle.get();
    if (new_circle_ptr) {
        new_circle_ptr->brain = parent_brain_copy;
        new_circle_ptr->set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
        new_circle_ptr->set_linear_damping(game.get_linear_damping(), worldId);
        new_circle_ptr->set_angular_damping(game.get_angular_damping(), worldId);
        new_circle_ptr->setAngle(angle + 3.14159f, worldId);
        new_circle_ptr->apply_forward_impulse();
        new_circle_ptr->update_color_from_brain();
        // Keep the original creation age so lineage age persists across divisions.
        new_circle_ptr->set_creation_time(get_creation_time());
        new_circle_ptr->set_last_division_time(game.get_sim_time());
    }

    this->set_generation(next_generation);
    if (new_circle_ptr) {
        new_circle_ptr->set_generation(next_generation);
    }
    owner_game = &game;
    set_last_division_time(game.get_sim_time());

    game.update_max_generation_from_circle(this);
    game.update_max_generation_from_circle(new_circle_ptr);

    this->apply_forward_impulse();

    const int mutation_rounds = std::max(0, game.get_mutation_rounds());
    float weight_thresh = game.get_mutate_weight_thresh();
    float weight_full = game.get_mutate_weight_full_change_thresh();
    float weight_factor = game.get_mutate_weight_factor();
    float reactivate = game.get_mutate_reactivate_connection_thresh();
    int add_conn_iters = game.get_mutate_add_connection_iterations();
    int add_node_iters = game.get_mutate_add_node_iterations();
    bool allow_recurrent = game.get_mutate_allow_recurrent();
    for (int i = 0; i < mutation_rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
                allow_recurrent,
                weight_thresh,
                weight_full,
                weight_factor,
                game.get_add_connection_probability(),
                add_conn_iters,
                reactivate,
                game.get_add_node_probability(),
                add_node_iters);
        }
        if (new_circle_ptr && new_circle_ptr->neat_innovations && new_circle_ptr->neat_last_innov_id) {
            new_circle_ptr->brain.mutate(
                new_circle_ptr->neat_innovations,
                new_circle_ptr->neat_last_innov_id,
                allow_recurrent,
                weight_thresh,
                weight_full,
                weight_factor,
                game.get_add_connection_probability(),
                add_conn_iters,
                reactivate,
                game.get_add_node_probability(),
                add_node_iters);
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
    std::array<float, SENSOR_COUNT> weights{};

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

        // Split an angular interval that may wrap into one or two non-wrapping segments in [-PI, PI].
        auto split_interval = [&](float start, float end) {
            std::vector<std::pair<float, float>> segments;
            start = normalize_angle(start);
            end = normalize_angle(end);
            if (end < start) {
                // Wrapped past PI -> split
                segments.emplace_back(start, PI);
                segments.emplace_back(-PI, end);
            } else {
                segments.emplace_back(start, end);
            }
            return segments;
        };

        auto overlap_length = [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
            float start = std::max(a.first, b.first);
            float end = std::min(a.second, b.second);
            return std::max(0.0f, end - start);
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
                // Coincident centers: distribute by sector angular size.
                const auto color = drawable->get_color_rgb();
                float w = std::max(circle->getArea(), 0.0f);
                float per_sector_w = w * (SECTOR_WIDTH / (2.0f * PI));
                for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
                    summed_colors[sector][0] += color[0] * per_sector_w;
                    summed_colors[sector][1] += color[1] * per_sector_w;
                    summed_colors[sector][2] += color[2] * per_sector_w;
                    weights[sector] += per_sector_w;
                }
                continue;
            }

            float relative_angle = std::atan2(dy, dx) - heading;
            relative_angle = normalize_angle(relative_angle);

            const auto color = drawable->get_color_rgb();
            float area = std::max(circle->getArea(), 0.0f);
            float distance = std::sqrt(dx * dx + dy * dy);
            float other_r = circle->getRadius();

            float half_span = PI; // default covers everything if overlapping deeply
            if (distance > other_r) {
                float ratio = std::clamp(other_r / distance, -1.0f, 1.0f);
                half_span = std::asin(ratio);
            }

            float span_start = relative_angle - half_span;
            float span_end = relative_angle + half_span;
            auto span_segments = split_interval(span_start, span_end);

            // Build sector segments once per circle check
            std::array<std::vector<std::pair<float, float>>, SENSOR_COUNT> sector_segments;
            for (int i = 0; i < SENSOR_COUNT; ++i) {
                float s_start = -SECTOR_HALF + i * SECTOR_WIDTH;
                float s_end = s_start + SECTOR_WIDTH;
                sector_segments[i] = split_interval(s_start, s_end);
            }

            std::array<float, SENSOR_COUNT> overlap_angles{};
            for (int i = 0; i < SENSOR_COUNT; ++i) {
                float total_overlap = 0.0f;
                for (const auto& ss : sector_segments[i]) {
                    for (const auto& sp : span_segments) {
                        total_overlap += overlap_length(ss, sp);
                    }
                }
                overlap_angles[i] = total_overlap;
            }

            for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
                float ang = overlap_angles[sector];
                if (ang <= 0.0f) continue;
                float w_sector = area * (ang / (2.0f * PI));
                summed_colors[sector][0] += color[0] * w_sector;
                summed_colors[sector][1] += color[1] * w_sector;
                summed_colors[sector][2] += color[2] * w_sector;
                weights[sector] += w_sector;
            }
        }
    }

    auto set_inputs_for_sector = [&](int base_index, const std::array<float, 3>& color_sum, float weight) {
        if (weight > 0.0f) {
            float inv = 1.0f / weight;
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
        set_inputs_for_sector(i * 3, summed_colors[i], weights[i]);
    }

    // Size input at the end: normalized area to [0,1]
    float area = this->getArea();
    float normalized = area / (area + 25.0f); // gentler saturation for larger sizes
    brain_inputs[24] = normalized;

    // Memory inputs (indices 25-28)
    for (int i = 0; i < 4; ++i) {
        brain_inputs[25 + i] = memory_state[i];
    }
}
