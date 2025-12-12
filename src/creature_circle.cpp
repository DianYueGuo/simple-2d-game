#include "creature_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <cmath>

#include <cstdlib>

namespace {
constexpr float PI = 3.14159f;
constexpr float TWO_PI = PI * 2.0f;
constexpr int SENSOR_COUNT = 8;
constexpr float SECTOR_WIDTH = PI / 4.0f;
constexpr float SECTOR_HALF = SECTOR_WIDTH * 0.5f;

using SectorSegment = std::pair<float, float>;
struct SpanSegments {
    std::array<SectorSegment, 2> segments{};
    int count = 0;
};
using SectorSegments = std::array<SpanSegments, SENSOR_COUNT>;
using SensorColors = std::array<std::array<float, 3>, SENSOR_COUNT>;
using SensorWeights = std::array<float, SENSOR_COUNT>;

float neat_activation(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float normalize_angle(float angle) {
    angle = std::fmod(angle, TWO_PI);
    if (angle > PI) {
        angle -= TWO_PI;
    } else if (angle < -PI) {
        angle += TWO_PI;
    }
    return angle;
}

SpanSegments split_interval(float start, float end) {
    SpanSegments segments{};
    start = normalize_angle(start);
    end = normalize_angle(end);
    if (end < start) {
        segments.segments[0] = {start, PI};
        segments.segments[1] = {-PI, end};
        segments.count = 2;
    } else {
        segments.segments[0] = {start, end};
        segments.count = 1;
    }
    return segments;
}

float overlap_length(const SectorSegment& a, const SectorSegment& b) {
    float start = std::max(a.first, b.first);
    float end = std::min(a.second, b.second);
    return std::max(0.0f, end - start);
}

const SectorSegments& get_sector_segments() {
    static const SectorSegments sector_segments = []() {
        SectorSegments result{};
        for (int i = 0; i < SENSOR_COUNT; ++i) {
            float s_start = -SECTOR_HALF + i * SECTOR_WIDTH;
            float s_end = s_start + SECTOR_WIDTH;
            result[i] = split_interval(s_start, s_end);
        }
        return result;
    }();
    return sector_segments;
}

float compute_half_span(float distance, float other_radius) {
    if (distance <= other_radius) {
        return PI;
    }
    float ratio = std::clamp(other_radius / distance, -1.0f, 1.0f);
    return std::asin(ratio);
}

SpanSegments build_span_segments(float relative_angle, float half_span) {
    float span_start = relative_angle - half_span;
    float span_end = relative_angle + half_span;
    return split_interval(span_start, span_end);
}

void accumulate_coincident_circle(const DrawableCircle& drawable, float weight, SensorColors& summed_colors, SensorWeights& weights) {
    const auto& color = drawable.get_color_rgb();
    float per_sector_w = weight * (SECTOR_WIDTH / (2.0f * PI));
    for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
        summed_colors[sector][0] += color[0] * per_sector_w;
        summed_colors[sector][1] += color[1] * per_sector_w;
        summed_colors[sector][2] += color[2] * per_sector_w;
        weights[sector] += per_sector_w;
    }
}

void accumulate_offset_circle(const DrawableCircle& drawable,
                              float area,
                              const SpanSegments& span_segments,
                              const SectorSegments& sector_segments,
                              SensorColors& summed_colors,
                              SensorWeights& weights) {
    std::array<float, SENSOR_COUNT> overlap_angles{};
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        float total_overlap = 0.0f;
        const auto& sector = sector_segments[i];
        for (int ss_idx = 0; ss_idx < sector.count; ++ss_idx) {
            const auto& ss = sector.segments[ss_idx];
            for (int sp_idx = 0; sp_idx < span_segments.count; ++sp_idx) {
                const auto& sp = span_segments.segments[sp_idx];
                total_overlap += overlap_length(ss, sp);
            }
        }
        overlap_angles[i] = total_overlap;
    }

    const auto& color = drawable.get_color_rgb();
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
} // namespace

CreatureCircle::CreatureCircle(const b2WorldId &worldId,
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
    EatableCircle(worldId, position_x, position_y, radius, density, /*toxic=*/false, /*division_pellet=*/false, angle, /*boost_particle=*/false),
    brain(base_brain ? *base_brain : neat::Genome(29, 11, 0, 0.5f, innov_ids, last_innov_id)) {
    set_kind(CircleKind::Creature);
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
    run_brain_cycle_from_touching();
    smooth_display_color(1.0f); // start display at brain-driven color immediately
}

float calculate_overlap_area(float r1, float r2, float distance) {
    if (distance >= r1 + r2) return 0.0f;
    if (distance <= fabs(r1 - r2)) return PI * fmin(r1, r2) * fmin(r1, r2);

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

void CreatureCircle::process_eating(const b2WorldId &worldId, Game& game, float poison_death_probability_toxic, float poison_death_probability_normal) {
    poisoned = false;
    for_each_touching([&](CirclePhysics& touching_circle) {
        if (!can_eat_circle(touching_circle)) {
            return;
        }
        auto* eatable = dynamic_cast<EatableCircle*>(&touching_circle);
        if (!eatable) {
            return;
        }
        if (eatable->is_eaten()) {
            return;
        }
        if (!has_overlap_to_eat(touching_circle)) {
            return;
        }
        float touching_area = eatable->getArea();
        consume_touching_circle(worldId, game, *eatable, touching_area, poison_death_probability_toxic, poison_death_probability_normal);
    });

    if (poisoned) {
        this->be_eaten();
    }
}

bool CreatureCircle::can_eat_circle(const CirclePhysics& circle) const {
    return circle.getRadius() < this->getRadius();
}

bool CreatureCircle::has_overlap_to_eat(const CirclePhysics& circle) const {
    float touching_area = circle.getArea();
    float overlap_threshold = touching_area * 0.8f;

    float distance = b2Distance(this->getPosition(), circle.getPosition());
    float overlap_area = calculate_overlap_area(this->getRadius(), circle.getRadius(), distance);

    return overlap_area >= overlap_threshold;
}

void CreatureCircle::consume_touching_circle(const b2WorldId &worldId, Game& game, EatableCircle& eatable, float touching_area, float poison_death_probability_toxic, float poison_death_probability_normal) {
    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (eatable.is_toxic()) {
        if (roll < poison_death_probability_toxic) {
            poisoned = true;
        }
        eatable.be_eaten();
        eatable.set_eaten_by(this);
    } else {
        if (roll < poison_death_probability_normal) {
            poisoned = true;
        }
        eatable.be_eaten();
        eatable.set_eaten_by(this);
        if (eatable.is_division_pellet()) {
            float div_roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
            if (div_roll <= game.get_division_pellet_divide_probability()) {
                this->divide(worldId, game);
            }
        }
    }

    this->grow_by_area(touching_area, worldId);
}

void CreatureCircle::move_randomly(const b2WorldId &worldId, Game &game) {
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

void CreatureCircle::run_brain_cycle_from_touching() {
    update_brain_inputs_from_touching();
    brain.loadInputs(brain_inputs.data());
    brain.runNetwork(neat_activation);
    brain.getOutputs(brain_outputs.data());
    update_color_from_brain();
}

void CreatureCircle::move_intelligently(const b2WorldId &worldId, Game &game, float dt) {
    (void)dt;
    run_brain_cycle_from_touching();

    if (game.get_selected_creature() == this &&
        owner_game && owner_game->is_selected_creature_possessed()
    ) {
        float probability = static_cast<float>(rand()) / RAND_MAX;
        if (owner_game->get_left_key_down() && probability > 0.3f) {
            this->apply_left_turn_impulse();
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (owner_game->get_right_key_down() && probability > 0.3f) {
            this->apply_right_turn_impulse();
        }
        if (owner_game->get_up_key_down()) {
            this->boost_forward(worldId, game);
        }
        if (owner_game->get_space_key_down()) {
            this->divide(worldId, game);
        }
    } else {
        float probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[0] >= probability) {
            this->boost_forward(worldId, game);
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[1] >= probability) {
            this->apply_left_turn_impulse();
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[2] >= probability) {
            this->apply_right_turn_impulse();
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[3] >= probability) {
            this->divide(worldId, game);
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

void CreatureCircle::update_inactivity(float dt, float timeout) {
    if (dt <= 0.0f) return;
    inactivity_timer += dt;
    b2Vec2 velocity = getLinearVelocity();
    constexpr float vel_epsilon = 1e-3f;
    const bool is_moving = (std::fabs(velocity.x) > vel_epsilon) || (std::fabs(velocity.y) > vel_epsilon);
    if (is_moving) {
        inactivity_timer = 0.0f;
        return;
    }
    if (timeout <= 0.0f) {
        // Inactivity timeout disabled.
        inactivity_timer = 0.0f;
        return;
    }
    if (inactivity_timer >= timeout && !is_eaten()) {
        poisoned = true;
        this->be_eaten();
        inactivity_timer = 0.0f;
    }
}

void CreatureCircle::boost_forward(const b2WorldId &worldId, Game& game) {
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

        float boost_radius = sqrt(boost_cost / PI);
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
            /*division_pellet=*/false,
            /*angle=*/0.0f,
            /*boost_particle=*/true);
        EatableCircle* boost_circle_ptr = boost_circle.get();
        const auto creature_signal_color = get_color_rgb(); // use true signal, not smoothed display
        boost_circle_ptr->set_color_rgb(creature_signal_color[0], creature_signal_color[1], creature_signal_color[2]);
        boost_circle_ptr->smooth_display_color(1.0f);
        float frac = game.get_boost_particle_impulse_fraction();
        boost_circle_ptr->set_impulse_magnitudes(game.get_linear_impulse_magnitude() * frac, game.get_angular_impulse_magnitude() * frac);
        boost_circle_ptr->set_linear_damping(game.get_boost_particle_linear_damping(), worldId);
        boost_circle_ptr->set_angular_damping(game.get_angular_damping(), worldId);
        game.add_circle(std::move(boost_circle));
        boost_circle_ptr->setAngle(angle + PI, worldId);
        boost_circle_ptr->apply_forward_impulse();
    }
}

void CreatureCircle::initialize_brain(int mutation_rounds, float add_node_p, float /*remove_node_p*/, float add_connection_p, float /*remove_connection_p*/) {
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

void CreatureCircle::divide(const b2WorldId &worldId, Game& game) {
    const float current_area = this->getArea();
    const float divided_area = current_area / 2.0f;

    if (divided_area <= minimum_area) {
        return;
    }

    const float new_radius = std::sqrt(divided_area / PI);

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

    auto new_circle = std::make_unique<CreatureCircle>(
        worldId,
        child_position.x,
        child_position.y,
        new_radius,
        game.get_circle_density(),
        angle + PI,
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
    CreatureCircle* new_circle_ptr = new_circle.get();
    if (new_circle_ptr) {
        configure_child_after_division(*new_circle_ptr, worldId, game, angle, parent_brain_copy);
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

    mutate_lineage(game, new_circle_ptr);

    update_color_from_brain();
    game.add_circle(std::move(new_circle));
}

void CreatureCircle::configure_child_after_division(CreatureCircle& child, const b2WorldId& worldId, const Game& game, float angle, const neat::Genome& parent_brain_copy) const {
    child.brain = parent_brain_copy;
    child.set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
    child.set_linear_damping(game.get_linear_damping(), worldId);
    child.set_angular_damping(game.get_angular_damping(), worldId);
    child.setAngle(angle + PI, worldId);
    child.apply_forward_impulse();
    child.update_color_from_brain();
    // Keep the original creation age so lineage age persists across divisions.
    child.set_creation_time(get_creation_time());
    child.set_last_division_time(game.get_sim_time());
}

void CreatureCircle::mutate_lineage(const Game& game, CreatureCircle* child) {
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
        if (child && child->neat_innovations && child->neat_last_innov_id) {
            child->brain.mutate(
                child->neat_innovations,
                child->neat_last_innov_id,
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
}

void CreatureCircle::update_color_from_brain() {
    float target_r = std::clamp(brain_outputs[4], 0.0f, 1.0f);
    float target_g = std::clamp(brain_outputs[5], 0.0f, 1.0f);
    float target_b = std::clamp(brain_outputs[6], 0.0f, 1.0f);
    set_color_rgb(target_r, target_g, target_b); // keep the signal exact
    constexpr float smoothing = 0.1f; // exponential smoothing factor for display only
    smooth_display_color(smoothing);
}

void CreatureCircle::update_brain_inputs_from_touching() {
    SensorColors summed_colors{};
    SensorWeights weights{};

    if (!touching_circles.empty()) {
        const b2Vec2 self_pos = this->getPosition();
        const float heading = this->getAngle();
        const auto& sector_segments = get_sector_segments();

        for_each_touching([&](const CirclePhysics& circle) {
            auto* drawable = dynamic_cast<const DrawableCircle*>(&circle);
            if (!drawable) {
                return;
            }

            const b2Vec2 other_pos = circle.getPosition();
            const float dx = other_pos.x - self_pos.x;
            const float dy = other_pos.y - self_pos.y;
            const float area = std::max(circle.getArea(), 0.0f);

            if (dx == 0.0f && dy == 0.0f) {
                accumulate_coincident_circle(*drawable, area, summed_colors, weights);
                return;
            }

            float relative_angle = normalize_angle(std::atan2(dy, dx) - heading);
            float distance = std::sqrt(dx * dx + dy * dy);
            float other_r = circle.getRadius();
            float half_span = compute_half_span(distance, other_r);

            auto span_segments = build_span_segments(relative_angle, half_span);
            accumulate_offset_circle(*drawable, area, span_segments, sector_segments, summed_colors, weights);
        });
    }

    apply_sensor_inputs(summed_colors, weights);
    write_size_and_memory_inputs();
}

void CreatureCircle::apply_sensor_inputs(const std::array<std::array<float, 3>, 8>& summed_colors, const std::array<float, 8>& weights) {
    // Order sensors clockwise starting from front: front, front-right, right, back-right, back, back-left, left, front-left.
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        int base_index = i * 3;
        float weight = weights[i];
        if (weight > 0.0f) {
            float inv = 1.0f / weight;
            brain_inputs[base_index]     = summed_colors[i][0] * inv;
            brain_inputs[base_index + 1] = summed_colors[i][1] * inv;
            brain_inputs[base_index + 2] = summed_colors[i][2] * inv;
        } else {
            brain_inputs[base_index]     = 0.0f;
            brain_inputs[base_index + 1] = 0.0f;
            brain_inputs[base_index + 2] = 0.0f;
        }
    }
}

void CreatureCircle::write_size_and_memory_inputs() {
    float area = this->getArea();
    float normalized = area / (area + 25.0f); // gentler saturation for larger sizes
    brain_inputs[24] = normalized;

    for (int i = 0; i < 4; ++i) {
        brain_inputs[25 + i] = memory_state[i];
    }
}
