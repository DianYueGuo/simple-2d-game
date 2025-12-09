#include "game/spawner.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>

#include "eater_circle.hpp"
#include "game.hpp"

namespace {
constexpr float PI = 3.14159f;

inline float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

inline float radius_from_area(float area) {
    return std::sqrt(std::max(area, 0.0f) / PI);
}
} // namespace

Spawner::Spawner(Game& game_ref) : game(game_ref) {}

void Spawner::try_add_circle_at(const sf::Vector2f& worldPos) {
    switch (game.get_add_type()) {
        case Game::AddType::Eater:
            if (auto circle = create_eater_at({worldPos.x, worldPos.y})) {
                game.update_max_generation_from_circle(circle.get());
                game.circles.push_back(std::move(circle));
            }
            break;
        case Game::AddType::Eatable:
        case Game::AddType::ToxicEatable:
        case Game::AddType::DivisionEatable:
            game.circles.push_back(create_eatable_at({worldPos.x, worldPos.y}, game.get_add_type() == Game::AddType::ToxicEatable, game.get_add_type() == Game::AddType::DivisionEatable));
            break;
        default:
            break;
    }
}

void Spawner::begin_add_drag_if_applicable(const sf::Vector2f& worldPos) {
    if (game.get_add_type() == Game::AddType::Eater) {
        reset_add_drag_state();
        return;
    }
    add_dragging = true;
    last_add_world_pos = worldPos;
    last_drag_world_pos = worldPos;
    add_drag_distance = 0.0f;
}

void Spawner::continue_add_drag(const sf::Vector2f& worldPos) {
    if (!add_dragging || game.get_cursor_mode() != Game::CursorMode::Add) {
        return;
    }

    if (!last_drag_world_pos) {
        last_drag_world_pos = worldPos;
    }

    float dx_move = worldPos.x - last_drag_world_pos->x;
    float dy_move = worldPos.y - last_drag_world_pos->y;
    add_drag_distance += std::sqrt(dx_move * dx_move + dy_move * dy_move);
    last_drag_world_pos = worldPos;

    const float min_spacing = radius_from_area(game.get_add_eatable_area()) * 2.0f;

    if (add_drag_distance >= min_spacing) {
        switch (game.get_add_type()) {
            case Game::AddType::Eater:
                break;
            case Game::AddType::Eatable:
            case Game::AddType::ToxicEatable:
            case Game::AddType::DivisionEatable:
                game.circles.push_back(create_eatable_at({worldPos.x, worldPos.y}, game.get_add_type() == Game::AddType::ToxicEatable, true));
                last_add_world_pos = worldPos;
                break;
        }
        add_drag_distance = 0.0f;
    }
}

void Spawner::reset_add_drag_state() {
    add_dragging = false;
    last_add_world_pos.reset();
    last_drag_world_pos.reset();
    add_drag_distance = 0.0f;
}

void Spawner::sprinkle_entities(float dt) {
    ensure_minimum_eaters();
    sprinkle_with_rate(game.get_sprinkle_rate_eatable(), static_cast<int>(Game::AddType::Eatable), dt);
    sprinkle_with_rate(game.get_sprinkle_rate_toxic(), static_cast<int>(Game::AddType::ToxicEatable), dt);
    sprinkle_with_rate(game.get_sprinkle_rate_division(), static_cast<int>(Game::AddType::DivisionEatable), dt);
}

void Spawner::ensure_minimum_eaters() {
    int eater_count = static_cast<int>(game.get_eater_count());
    if (eater_count >= game.get_minimum_eater_count()) {
        return;
    }

    int to_spawn = game.get_minimum_eater_count() - eater_count;
    for (int i = 0; i < to_spawn; ++i) {
        if (auto eater = create_eater_at(random_point_in_petri())) {
            game.update_max_generation_from_circle(eater.get());
            game.circles.push_back(std::move(eater));
        }
    }
}

b2Vec2 Spawner::random_point_in_petri() const {
    float angle = random_unit() * 2.0f * PI;
    float radius = game.get_petri_radius() * std::sqrt(random_unit());
    return b2Vec2{radius * std::cos(angle), radius * std::sin(angle)};
}

std::unique_ptr<EaterCircle> Spawner::create_eater_at(const b2Vec2& pos) {
    float base_area = std::max(game.get_average_eater_area(), 0.0001f);
    float varied_area = base_area * (0.5f + random_unit()); // random scale around the average
    float radius = radius_from_area(varied_area);
    float angle = random_unit() * 2.0f * PI;
    const neat::Genome* base_brain = game.get_max_generation_brain();
    auto circle = std::make_unique<EaterCircle>(
        game.worldId,
        pos.x,
        pos.y,
        radius,
        game.get_circle_density(),
        angle,
        0,
        game.get_init_mutation_rounds(),
        game.get_init_add_node_probability(),
        game.get_init_remove_node_probability(),
        game.get_init_add_connection_probability(),
        game.get_init_remove_connection_probability(),
        base_brain,
        game.get_neat_innovations(),
        game.get_neat_last_innovation_id(),
        &game);
    circle->set_creation_time(game.get_sim_time());
    circle->set_last_division_time(game.get_sim_time());
    circle->set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
    circle->set_linear_damping(game.get_linear_damping(), game.worldId);
    circle->set_angular_damping(game.get_angular_damping(), game.worldId);
    return circle;
}

std::unique_ptr<EatableCircle> Spawner::create_eatable_at(const b2Vec2& pos, bool toxic, bool division_boost) const {
    float radius = radius_from_area(game.get_add_eatable_area());
    auto circle = std::make_unique<EatableCircle>(game.worldId, pos.x, pos.y, radius, game.get_circle_density(), toxic, division_boost, 0.0f);
    circle->set_impulse_magnitudes(game.get_linear_impulse_magnitude(), game.get_angular_impulse_magnitude());
    circle->set_linear_damping(game.get_linear_damping(), game.worldId);
    circle->set_angular_damping(game.get_angular_damping(), game.worldId);
    return circle;
}

void Spawner::spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out) {
    float eater_radius = eater.getRadius();
    float total_area = eater.getArea();
    if (game.get_minimum_area() <= 0.0f || total_area <= 0.0f) {
        return;
    }

    float chunk_area = std::min(game.get_minimum_area(), total_area);
    float remaining_area = total_area * (std::clamp(game.get_eater_cloud_area_percentage(), 0.0f, 100.0f) / 100.0f);

    while (remaining_area > 0.0f) {
        float use_area = std::min(chunk_area, remaining_area);
        float piece_radius = radius_from_area(use_area);
        float max_offset = std::max(0.0f, eater_radius - piece_radius);

        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * PI;
        float dist = max_offset * std::sqrt(random_unit());
        b2Vec2 pos = eater.getPosition();
        b2Vec2 piece_pos = {pos.x + std::cos(angle) * dist, pos.y + std::sin(angle) * dist};

        out.push_back(create_eatable_at(piece_pos, false));

        remaining_area -= use_area;
    }
}

void Spawner::sprinkle_with_rate(float rate, int type_value, float dt) {
    if (rate <= 0.0f || dt <= 0.0f || game.get_petri_radius() <= 0.0f) {
        return;
    }

    Game::AddType type = static_cast<Game::AddType>(type_value);

    float expected = rate * dt;
    int guaranteed = static_cast<int>(expected);
    float remainder = expected - static_cast<float>(guaranteed);

    auto spawn_once = [&]() {
        b2Vec2 pos = random_point_in_petri();
        switch (type) {
            case Game::AddType::Eater:
                if (auto eater = create_eater_at(pos)) {
                    game.update_max_generation_from_circle(eater.get());
                    game.circles.push_back(std::move(eater));
                }
                break;
            case Game::AddType::Eatable:
                game.circles.push_back(create_eatable_at(pos, false));
                break;
            case Game::AddType::ToxicEatable:
                game.circles.push_back(create_eatable_at(pos, true));
                break;
            case Game::AddType::DivisionEatable:
                game.circles.push_back(create_eatable_at(pos, false, true));
                break;
        }
    };

    for (int i = 0; i < guaranteed; ++i) {
        spawn_once();
    }

    float roll = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    if (roll < remainder) {
        spawn_once();
    }
}
