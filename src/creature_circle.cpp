#include "creature_circle.hpp"
#include "game.hpp"

#include <algorithm>
#include <cmath>

#include <cstdlib>
#include <vector>

namespace {
constexpr float PI = 3.14159f;
constexpr float TWO_PI = PI * 2.0f;
constexpr int SENSOR_COUNT = kColorSensorCount;
static_assert(SENSOR_COUNT >= kMinColorSensorCount && SENSOR_COUNT <= kMaxColorSensorCount, "Color sensor count out of supported range.");
constexpr float SECTOR_WIDTH = TWO_PI / static_cast<float>(SENSOR_COUNT);
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

float cross(const b2Vec2& a, const b2Vec2& b) {
    return a.x * b.y - a.y * b.x;
}

float dot(const b2Vec2& a, const b2Vec2& b) {
    return a.x * b.x + a.y * b.y;
}

float triangle_circle_intersection_area(const b2Vec2& a, const b2Vec2& b, float radius) {
    // Circle is centered at the origin in this helper.
    const float r2 = radius * radius;
    const float len_a2 = dot(a, a);
    const float EPS = 1e-6f;

    // If both vertices are effectively at the origin, there is no area.
    if (len_a2 < EPS && dot(b, b) < EPS) {
        return 0.0f;
    }

    struct ParamPoint {
        float t;
        b2Vec2 p;
    };
    std::array<ParamPoint, 4> pts{};
    int count = 0;
    pts[count++] = {0.0f, a};

    // Solve for intersections of segment ab with the circle.
    b2Vec2 d{b.x - a.x, b.y - a.y};
    float A = dot(d, d);
    float B = 2.0f * dot(a, d);
    float C = len_a2 - r2;
    float disc = B * B - 4.0f * A * C;
    if (disc >= 0.0f && A > EPS) {
        float sqrt_disc = std::sqrt(disc);
        float inv_denom = 0.5f / A;
        float t1 = (-B - sqrt_disc) * inv_denom;
        float t2 = (-B + sqrt_disc) * inv_denom;
        if (t1 > t2) std::swap(t1, t2);
        if (t1 > EPS && t1 < 1.0f - EPS) {
            pts[count++] = {t1, b2Vec2{a.x + d.x * t1, a.y + d.y * t1}};
        }
        if (t2 > EPS && t2 < 1.0f - EPS && std::fabs(t2 - t1) > EPS) {
            if (count < static_cast<int>(pts.size())) {
                pts[count++] = {t2, b2Vec2{a.x + d.x * t2, a.y + d.y * t2}};
            }
        }
    }

    pts[count++] = {1.0f, b};
    std::sort(pts.begin(), pts.begin() + count, [&](const ParamPoint& p1, const ParamPoint& p2) {
        return p1.t < p2.t;
    });

    float area = 0.0f;
    for (int i = 0; i + 1 < count; ++i) {
        const b2Vec2& p = pts[i].p;
        const b2Vec2& q = pts[i + 1].p;
        b2Vec2 mid{0.5f * (p.x + q.x), 0.5f * (p.y + q.y)};
        const float mid_len2 = dot(mid, mid);
        if (mid_len2 <= r2 + EPS) {
            area += 0.5f * cross(p, q);
        } else {
            float ang = std::atan2(cross(p, q), dot(p, q));
            area += 0.5f * r2 * ang;
        }
    }

    return area;
}

float circle_triangle_intersection_area(const std::array<b2Vec2, 3>& poly, const b2Vec2& center, float radius) {
    // Translate polygon so circle center is at the origin.
    float area = 0.0f;
    for (int i = 0; i < 3; ++i) {
        b2Vec2 a{poly[i].x - center.x, poly[i].y - center.y};
        const auto& next = poly[(i + 1) % 3];
        b2Vec2 b{next.x - center.x, next.y - center.y};
        area += triangle_circle_intersection_area(a, b, radius);
    }
    return area;
}

float circle_wedge_overlap_area(const b2Vec2& circle_center_local, float radius, float start_angle, float end_angle) {
    // Build a large triangle that represents the wedge; large enough to fully contain the circle footprint.
    float dist_to_origin = std::sqrt(circle_center_local.x * circle_center_local.x + circle_center_local.y * circle_center_local.y);
    float ray_length = dist_to_origin + radius + 1.0f; // add slack to guarantee containment

    b2Vec2 p1{std::cos(start_angle) * ray_length, std::sin(start_angle) * ray_length};
    b2Vec2 p2{std::cos(end_angle) * ray_length, std::sin(end_angle) * ray_length};
    std::array<b2Vec2, 3> triangle{{b2Vec2{0.0f, 0.0f}, p1, p2}};

    float area = circle_triangle_intersection_area(triangle, circle_center_local, radius);
    return std::max(0.0f, area);
}

float normalize_angle_positive(float angle) {
    float a = std::fmod(angle, TWO_PI);
    if (a < 0.0f) {
        a += TWO_PI;
    }
    return a;
}

void accumulate_touching_circle(const CirclePhysics& circle,
                                const DrawableCircle& drawable,
                                const b2Vec2& self_pos,
                                float cos_h,
                                float sin_h,
                                const SectorSegments& sector_segments,
                                SensorColors& summed_colors,
                                SensorWeights& weights) {
    const b2Vec2 other_pos = circle.getPosition();
    b2Vec2 rel_world{other_pos.x - self_pos.x, other_pos.y - self_pos.y};
    b2Vec2 rel_local{
        cos_h * rel_world.x + sin_h * rel_world.y,
        -sin_h * rel_world.x + cos_h * rel_world.y
    };

    const float other_r = circle.getRadius();
    const float dist2 = rel_local.x * rel_local.x + rel_local.y * rel_local.y;
    const float other_r2 = other_r * other_r;
    const auto& color = drawable.get_color_rgb();

    const auto clamp_sector_index = [](int idx) {
        return std::clamp(idx, 0, SENSOR_COUNT - 1);
    };

    auto accumulate_sector = [&](int sector) {
        const int clamped_sector = clamp_sector_index(sector);
        float area_in_sector = 0.0f;
        const auto& segs = sector_segments[clamped_sector];
        for (int idx = 0; idx < segs.count; ++idx) {
            const auto& seg = segs.segments[idx];
            area_in_sector += circle_wedge_overlap_area(rel_local, other_r, seg.first, seg.second);
        }

        if (area_in_sector <= 0.0f) {
            return;
        }

        summed_colors[clamped_sector][0] += color[0] * area_in_sector;
        summed_colors[clamped_sector][1] += color[1] * area_in_sector;
        summed_colors[clamped_sector][2] += color[2] * area_in_sector;
        weights[clamped_sector] += area_in_sector;
    };

    if (dist2 <= other_r2) {
        // Circle encompasses the origin; intersects all sectors.
        for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
            accumulate_sector(sector);
        }
        return;
    }

    const float dist = std::sqrt(dist2);
    const float half_span = std::asin(std::clamp(other_r / dist, 0.0f, 1.0f));
    const float center_angle = std::atan2(rel_local.y, rel_local.x);
    constexpr float pad = 1e-4f; // avoid missing boundary-touching sectors
    float start = normalize_angle_positive(center_angle - half_span - pad);
    float end = normalize_angle_positive(center_angle + half_span + pad);

    auto angle_to_index = [&](float angle) {
        int idx = static_cast<int>(std::floor(angle / SECTOR_WIDTH));
        return clamp_sector_index(idx);
    };

    int start_idx = angle_to_index(start);
    int end_idx = angle_to_index(end);

    auto process_range = [&](int from, int to) {
        for (int s = from; ; ++s) {
            accumulate_sector(s);
            if (s == to) break;
        }
    };

    if (start <= end) {
        process_range(start_idx, end_idx);
    } else {
        process_range(start_idx, SENSOR_COUNT - 1);
        process_range(0, end_idx);
    }
}

void accumulate_outside_petri(const b2Vec2& self_pos,
                              float self_radius,
                              float cos_h,
                              float sin_h,
                              float petri_radius,
                              const SectorSegments& sector_segments,
                              SensorColors& summed_colors,
                              SensorWeights& weights) {
    if (petri_radius <= 0.0f || self_radius <= 0.0f) {
        return;
    }

    // Petri dish is centered at the world origin.
    b2Vec2 rel_world{-self_pos.x, -self_pos.y};
    b2Vec2 dish_local{
        cos_h * rel_world.x + sin_h * rel_world.y,
        -sin_h * rel_world.x + cos_h * rel_world.y
    };

    constexpr float epsilon = 1e-6f;
    for (int sector = 0; sector < SENSOR_COUNT; ++sector) {
        float outside_area = 0.0f;
        const auto& segs = sector_segments[sector];
        for (int idx = 0; idx < segs.count; ++idx) {
            const auto& seg = segs.segments[idx];
            float span = seg.second - seg.first;
            if (span <= 0.0f) {
                continue;
            }

            // Scale the ray length so the triangle area matches the circular sector area.
            float sin_span = std::sin(span);
            float ray_length = self_radius;
            if (std::fabs(sin_span) > epsilon) {
                ray_length = self_radius * std::sqrt(span / sin_span);
            }

            b2Vec2 p1{std::cos(seg.first) * ray_length, std::sin(seg.first) * ray_length};
            b2Vec2 p2{std::cos(seg.second) * ray_length, std::sin(seg.second) * ray_length};
            std::array<b2Vec2, 3> triangle{{b2Vec2{0.0f, 0.0f}, p1, p2}};

            float inside_area = circle_triangle_intersection_area(triangle, dish_local, petri_radius);
            float segment_area = 0.5f * self_radius * self_radius * span;
            inside_area = std::clamp(inside_area, 0.0f, segment_area);

            outside_area += segment_area - inside_area;
        }

        if (outside_area > 0.0f) {
            summed_colors[sector][0] += outside_area; // Sense outside as red.
            weights[sector] += outside_area;
        }
    }
}

void spawn_boost_particle(const b2WorldId& worldId,
                          Game& game,
                          const CreatureCircle& parent,
                          float boost_radius,
                          float angle,
                          const b2Vec2& back_position) {
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
    const auto creature_signal_color = parent.get_color_rgb(); // use true signal, not smoothed display
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
                         float init_add_connection_probability,
                         const neat::Genome* base_brain,
                         std::vector<std::vector<int>>* innov_ids,
                         int* last_innov_id,
                         Game* owner) :
    EatableCircle(worldId, position_x, position_y, radius, density, /*toxic=*/false, /*division_pellet=*/false, angle, /*boost_particle=*/false),
    brain(base_brain ? *base_brain : neat::Genome(BRAIN_INPUTS, BRAIN_OUTPUTS, innov_ids, last_innov_id, 0.001f)) {
    set_kind(CircleKind::Creature);
    neat_innovations = innov_ids;
    neat_last_innov_id = last_innov_id;
    owner_game = owner;
    set_generation(generation);
    initialize_brain(
        init_mutation_rounds,
        init_add_node_probability,
        init_add_connection_probability);
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
    const float touching_area = circle.getArea();
    const float overlap_threshold = touching_area * 0.8f;

    const float r_self = this->getRadius();
    const float r_other = circle.getRadius();

    const b2Vec2 self_pos = this->getPosition();
    const b2Vec2 other_pos = circle.getPosition();
    const float dx = self_pos.x - other_pos.x;
    const float dy = self_pos.y - other_pos.y;
    const float dist2 = dx * dx + dy * dy;

    const float sum_r = r_self + r_other;
    const float sum_r2 = sum_r * sum_r;
    if (dist2 >= sum_r2) {
        // No overlap possible.
        return false;
    }

    const float diff_r = r_self - r_other;
    const float diff_r2 = diff_r * diff_r;
    if (dist2 <= diff_r2) {
        // Smaller circle fully contained; overlap is entire touching area.
        return touching_area >= overlap_threshold;
    }

    const float distance = std::sqrt(dist2);
    float overlap_area = calculate_overlap_area(r_self, r_other, distance);

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
        this->boost_eccentric_forward_right(worldId, game);

    probability = static_cast<float>(rand()) / RAND_MAX;
    if (probability > 0.9f)
        this->boost_eccentric_forward_left(worldId, game);
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
        if (owner_game->get_left_key_down()) {
            this->boost_eccentric_forward_left(worldId, game);
        }
        if (owner_game->get_right_key_down()) {
            this->boost_eccentric_forward_right(worldId, game);
        }
        if (owner_game->get_space_key_down()) {
            this->divide(worldId, game);
        }
    } else {
        float probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[0] >= probability) {
            this->boost_eccentric_forward_left(worldId, game);
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[1] >= probability) {
            this->boost_eccentric_forward_right(worldId, game);
        }
        probability = static_cast<float>(rand()) / RAND_MAX;
        if (brain_outputs[2] >= probability) {
            this->divide(worldId, game);
        }
    }

    if (game.get_live_mutation_enabled() && neat_innovations && neat_last_innov_id) {
        brain.mutate(
            neat_innovations,
            neat_last_innov_id,
            game.get_mutate_weight_thresh(),
            game.get_mutate_weight_full_change_thresh(),
            game.get_mutate_weight_factor(),
            game.get_tick_add_connection_probability(),
            game.get_mutate_add_connection_iterations(),
            game.get_mutate_reactivate_connection_thresh(),
            game.get_tick_add_node_probability(),
            game.get_mutate_add_node_iterations());
    }

    // Update memory from dedicated memory outputs (clamped).
    // Memory outputs are distinct: outputs 6-9.
    for (int i = 0; i < MEMORY_SLOTS; ++i) {
        memory_state[i] = std::clamp(brain_outputs[6 + i], 0.0f, 1.0f);
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
        b2Vec2 back_position = {
            pos.x - direction.x * (this->getRadius() + boost_radius),
            pos.y - direction.y * (this->getRadius() + boost_radius)
        };

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, back_position);
    }
}

namespace {
b2Vec2 compute_lateral_boost_position(const CreatureCircle& creature, bool to_right) {
    b2Vec2 pos = creature.getPosition();
    float angle = creature.getAngle();
    b2Vec2 direction = {cos(angle), sin(angle)};
    b2Vec2 right_dir = {direction.y, -direction.x};
    constexpr float lateral_fraction = 0.5f; // exact lateral offset as a fraction of radius
    float lateral_sign = to_right ? 1.0f : -1.0f;
    float lat = std::clamp(lateral_fraction, 0.0f, 1.0f);
    float back = std::sqrt(std::max(0.0f, 1.0f - lat * lat));
    // Offset direction already unit length; scale to rim.
    b2Vec2 offset_dir = {
        -direction.x * back + right_dir.x * lat * lateral_sign,
        -direction.y * back + right_dir.y * lat * lateral_sign
    };
    float scale = creature.getRadius();
    return {
        pos.x + offset_dir.x * scale,
        pos.y + offset_dir.y * scale
    };
}
} // namespace

void CreatureCircle::boost_eccentric_forward_right(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(game.get_boost_area(), 0.0f);
    float new_area = current_area - boost_cost;

    float boost_radius = (boost_cost > 0.0f) ? sqrt(boost_cost / PI) : 0.0f;
    if (boost_cost <= 0.0f) {
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/true);
        this->apply_forward_impulse_at_point(boost_position);
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        float angle = this->getAngle();
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/true);
        this->apply_forward_impulse_at_point(boost_position);

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, boost_position);
    }
}

void CreatureCircle::boost_eccentric_forward_left(const b2WorldId &worldId, Game& game) {
    float current_area = this->getArea();
    float boost_cost = std::max(game.get_boost_area(), 0.0f);
    float new_area = current_area - boost_cost;

    float boost_radius = (boost_cost > 0.0f) ? sqrt(boost_cost / PI) : 0.0f;
    if (boost_cost <= 0.0f) {
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/false);
        this->apply_forward_impulse_at_point(boost_position);
        inactivity_timer = 0.0f;
        return;
    }

    if (new_area > minimum_area) {
        this->setArea(new_area, worldId);
        float angle = this->getAngle();
        b2Vec2 boost_position = compute_lateral_boost_position(*this, /*to_right=*/false);
        this->apply_forward_impulse_at_point(boost_position);

        spawn_boost_particle(worldId, game, *this, boost_radius, angle, boost_position);
    }
}

void CreatureCircle::initialize_brain(int mutation_rounds, float add_node_p, float add_connection_p) {
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
            if (owner_game) {
                weight_thresh = owner_game->get_mutate_weight_thresh();
                weight_full = owner_game->get_mutate_weight_full_change_thresh();
                weight_factor = owner_game->get_mutate_weight_factor();
                reactivate = owner_game->get_mutate_reactivate_connection_thresh();
                add_conn_iters = owner_game->get_mutate_add_connection_iterations();
                add_node_iters = owner_game->get_mutate_add_node_iterations();
            }
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
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

    if (!has_sufficient_area_for_division(divided_area)) {
        return;
    }

    const float new_radius = std::sqrt(divided_area / PI);
    neat::Genome parent_brain_copy = brain;

    const b2Vec2 original_pos = this->getPosition();
    const float angle = this->getAngle();
    const auto [parent_position, child_position] = calculate_division_positions(original_pos, angle, new_radius);

    this->setRadius(new_radius, worldId);
    this->setPosition(parent_position, worldId);

    const int next_generation = this->get_generation() + 1;
    auto new_circle = create_division_child(
        worldId,
        game,
        new_radius,
        angle,
        next_generation,
        child_position,
        parent_brain_copy);
    CreatureCircle* new_circle_ptr = new_circle.get();

    apply_post_division_updates(game, new_circle_ptr, next_generation);
    game.add_circle(std::move(new_circle));
}

bool CreatureCircle::has_sufficient_area_for_division(float divided_area) const {
    return divided_area > minimum_area;
}

std::pair<b2Vec2, b2Vec2> CreatureCircle::calculate_division_positions(const b2Vec2& original_pos, float angle, float new_radius) const {
    b2Vec2 direction = {std::cos(angle), std::sin(angle)};
    b2Vec2 forward_offset = {direction.x * new_radius, direction.y * new_radius};

    b2Vec2 parent_position = {original_pos.x + forward_offset.x, original_pos.y + forward_offset.y};
    b2Vec2 child_position = {original_pos.x - forward_offset.x, original_pos.y - forward_offset.y};
    return {parent_position, child_position};
}

std::unique_ptr<CreatureCircle> CreatureCircle::create_division_child(const b2WorldId& worldId,
                                                                      Game& game,
                                                                      float new_radius,
                                                                      float angle,
                                                                      int next_generation,
                                                                      const b2Vec2& child_position,
                                                                      const neat::Genome& parent_brain_copy) {
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
        game.get_init_add_connection_probability(),
        &brain,
        game.get_neat_innovations(),
        game.get_neat_last_innovation_id(),
        &game);

    if (new_circle) {
        configure_child_after_division(*new_circle, worldId, game, angle, parent_brain_copy);
    }

    return new_circle;
}

void CreatureCircle::apply_post_division_updates(Game& game, CreatureCircle* child, int next_generation) {
    this->set_generation(next_generation);
    if (child) {
        child->set_generation(next_generation);
    }

    owner_game = &game;
    set_last_division_time(game.get_sim_time());
    if (owner_game) {
        owner_game->mark_age_dirty();
    }

    game.update_max_generation_from_circle(this);
    game.update_max_generation_from_circle(child);

    this->apply_forward_impulse();

    mutate_lineage(game, child);

    update_color_from_brain();
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
    for (int i = 0; i < mutation_rounds; ++i) {
        if (neat_innovations && neat_last_innov_id) {
            brain.mutate(
                neat_innovations,
                neat_last_innov_id,
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
    float target_r = std::clamp(brain_outputs[3], 0.0f, 1.0f);
    float target_g = std::clamp(brain_outputs[4], 0.0f, 1.0f);
    float target_b = std::clamp(brain_outputs[5], 0.0f, 1.0f);
    set_color_rgb(target_r, target_g, target_b); // keep the signal exact
    constexpr float smoothing = 0.1f; // exponential smoothing factor for display only
    smooth_display_color(smoothing);
}

void CreatureCircle::update_brain_inputs_from_touching() {
    SensorColors summed_colors{};
    SensorWeights weights{};

    const b2Vec2 self_pos = this->getPosition();
    const float heading = this->getAngle();
    const float cos_h = std::cos(heading);
    const float sin_h = std::sin(heading);
    const auto& sector_segments = get_sector_segments();

    if (!touching_circles.empty()) {
        for_each_touching([&](const CirclePhysics& circle) {
            auto* drawable = dynamic_cast<const DrawableCircle*>(&circle);
            if (!drawable) {
                return;
            }
            accumulate_touching_circle(circle, *drawable, self_pos, cos_h, sin_h, sector_segments, summed_colors, weights);
        });
    }

    if (owner_game) {
        float petri_radius = owner_game->get_petri_radius();
        accumulate_outside_petri(self_pos, getRadius(), cos_h, sin_h, petri_radius, sector_segments, summed_colors, weights);
    }

    apply_sensor_inputs(summed_colors, weights);
    write_size_and_memory_inputs();
}

void CreatureCircle::apply_sensor_inputs(const std::array<std::array<float, 3>, SENSOR_COUNT>& summed_colors, const std::array<float, SENSOR_COUNT>& weights) {
    // Order sensors clockwise starting from the forward-facing sector.
    const float sector_area = (PI * getRadius() * getRadius()) / static_cast<float>(SENSOR_COUNT);
    for (int i = 0; i < SENSOR_COUNT; ++i) {
        int base_index = i * 3;
        float weight = weights[i];
        if (weight > 0.0f) {
            float r_sum = summed_colors[i][0];
            float g_sum = summed_colors[i][1];
            float b_sum = summed_colors[i][2];
            float denom_r = r_sum + sector_area;
            float denom_g = g_sum + sector_area;
            float denom_b = b_sum + sector_area;
            brain_inputs[base_index]     = denom_r > 0.0f ? (r_sum / denom_r) : 0.0f;
            brain_inputs[base_index + 1] = denom_g > 0.0f ? (g_sum / denom_g) : 0.0f;
            brain_inputs[base_index + 2] = denom_b > 0.0f ? (b_sum / denom_b) : 0.0f;
        } else {
            brain_inputs[base_index]     = 0.0f;
            brain_inputs[base_index + 1] = 0.0f;
            brain_inputs[base_index + 2] = 0.0f;
        }
    }
}

void CreatureCircle::write_size_and_memory_inputs() {
    float area = this->getArea();
    float normalized = area / (area + 10.0f); // gentler saturation for larger sizes
    brain_inputs[SIZE_INPUT_INDEX] = normalized;

    for (int i = 0; i < MEMORY_SLOTS; ++i) {
        brain_inputs[MEMORY_INPUT_START + i] = memory_state[i];
    }
}
