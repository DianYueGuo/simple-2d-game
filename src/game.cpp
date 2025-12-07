#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <numeric>
#include <random>

#include "game.hpp"
#include "eater_circle.hpp"

namespace {
constexpr float PI = 3.14159f;
inline float random_unit() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}
inline float radius_from_area(float area) {
    return std::sqrt(std::max(area, 0.0f) / PI);
}
} // namespace

Game::Game() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
}

Game::~Game() {
    b2DestroyWorld(worldId);
}

void process_touch_events(const b2WorldId& worldId) {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        void* sensorShapeUserData = b2Shape_GetUserData(beginTouch->sensorShapeId);
        void* visitorShapeUserData = b2Shape_GetUserData(beginTouch->visitorShapeId);

        static_cast<CirclePhysics*>(sensorShapeUserData)->add_touching_circle(static_cast<CirclePhysics*>(visitorShapeUserData));
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        if (b2Shape_IsValid(endTouch->sensorShapeId) && b2Shape_IsValid(endTouch->visitorShapeId))
        {
            void* sensorShapeUserData = b2Shape_GetUserData(endTouch->sensorShapeId);
            void* visitorShapeUserData = b2Shape_GetUserData(endTouch->visitorShapeId);

            static_cast<CirclePhysics*>(sensorShapeUserData)->remove_touching_circle(static_cast<CirclePhysics*>(visitorShapeUserData));
        }
    }
}

void Game::process_game_logic() {
    if (paused) {
        return;
    }

    float timeStep = (1.0f / 60.0f) * time_scale;
    int subStepCount = 4;
    b2World_Step(worldId, timeStep, subStepCount);
    sim_time_accum += timeStep;
    // real_time_accum should be updated by caller using frame delta; leave as is here.

    process_touch_events(worldId);

    brain_time_accumulator += timeStep;
    sprinkle_entities(timeStep);
    update_eaters(worldId);
    run_brain_updates(worldId, timeStep);
    cull_consumed();
    remove_stopped_boost_particles();
    if (auto_remove_outside) {
        remove_outside_petri();
    }
    update_max_ages();
}

void Game::draw(sf::RenderWindow& window) const {
    // Draw petri dish boundary
    sf::CircleShape boundary(petri_radius * pixles_per_meter);
    boundary.setOrigin({petri_radius * pixles_per_meter, petri_radius * pixles_per_meter});
    boundary.setPosition({0.0f, 0.0f});
    boundary.setOutlineColor(sf::Color::White);
    boundary.setOutlineThickness(2.0f);
    boundary.setFillColor(sf::Color::Transparent);
    window.draw(boundary);

    for (const auto& circle : circles) {
        circle->draw(window, pixles_per_meter);
    }
}

void Game::process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event) {
    if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        handle_mouse_press(window, *mouseButtonPressed);
    }

    if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        handle_mouse_release(*mouseButtonReleased);
    }

    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        handle_mouse_move(window, *mouseMoved);
    }

    if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        sf::View view = window.getView();
        constexpr float zoom_factor = 1.05f;
        if (mouseWheel->delta > 0) {
            view.zoom(1.0f / zoom_factor);
        } else if (mouseWheel->delta < 0) {
            view.zoom(zoom_factor);
        }
        window.setView(view);
    }

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        handle_key_press(window, *keyPressed);
    }
}

void Game::handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e) {
    if (e.button == sf::Mouse::Button::Left) {
        sf::Vector2f viewPos = window.mapPixelToCoords(e.position);
        sf::Vector2f worldPos = {viewPos.x / pixles_per_meter, viewPos.y / pixles_per_meter};

        auto add_circle_at = [&](sf::Vector2f pos) {
            switch (add_type) {
                case AddType::Eater:
                    if (auto circle = create_eater_at({pos.x, pos.y})) {
                        update_max_generation_from_circle(circle.get());
                        circles.push_back(std::move(circle));
                    }
                    break;
                case AddType::Eatable:
                case AddType::ToxicEatable:
                    circles.push_back(create_eatable_at({pos.x, pos.y}, add_type == AddType::ToxicEatable));
                    break;
            }
        };

        if (cursor_mode == CursorMode::Add) {
            add_circle_at(worldPos);
            add_dragging = (add_type != AddType::Eater);
            if (add_dragging) {
                last_add_world_pos = worldPos;
                last_drag_world_pos = worldPos;
                add_drag_distance = 0.0f;
            } else {
                last_add_world_pos.reset();
                last_drag_world_pos.reset();
                add_drag_distance = 0.0f;
            }
        } else if (cursor_mode == CursorMode::Drag) {
            dragging = true;
            right_dragging = false;
            last_drag_pixels = e.position;
        } else if (cursor_mode == CursorMode::Select) {
            select_circle_at_world({worldPos.x, worldPos.y});
        }
    } else if (e.button == sf::Mouse::Button::Right) {
        dragging = true;
        right_dragging = true;
        last_drag_pixels = e.position;
    }
}

void Game::handle_mouse_release(const sf::Event::MouseButtonReleased& e) {
    if ((e.button == sf::Mouse::Button::Left && cursor_mode == CursorMode::Drag) ||
        e.button == sf::Mouse::Button::Right) {
        dragging = false;
        right_dragging = false;
    }
    if (e.button == sf::Mouse::Button::Left) {
        add_dragging = false;
        last_add_world_pos.reset();
        last_drag_world_pos.reset();
        add_drag_distance = 0.0f;
    }
}

void Game::handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    if (add_dragging && cursor_mode == CursorMode::Add) {
        sf::Vector2f viewPos = window.mapPixelToCoords({e.position.x, e.position.y});
        sf::Vector2f worldPos = {viewPos.x / pixles_per_meter, viewPos.y / pixles_per_meter};

        if (!last_drag_world_pos) {
            last_drag_world_pos = worldPos;
        }

        float dx_move = worldPos.x - last_drag_world_pos->x;
        float dy_move = worldPos.y - last_drag_world_pos->y;
        add_drag_distance += std::sqrt(dx_move * dx_move + dy_move * dy_move);
        last_drag_world_pos = worldPos;

        const float min_spacing = radius_from_area(add_eatable_area) * 2.0f;

        if (add_drag_distance >= min_spacing) {
            switch (add_type) {
                case AddType::Eater:
                    break;
                case AddType::Eatable:
                case AddType::ToxicEatable:
                    circles.push_back(create_eatable_at({worldPos.x, worldPos.y}, add_type == AddType::ToxicEatable));
                    last_add_world_pos = worldPos;
                    break;
            }
            add_drag_distance = 0.0f;
        }
    }

    if (dragging && (cursor_mode == CursorMode::Drag || right_dragging)) {
        sf::View view = window.getView();
        sf::Vector2f pixels_to_world = {
            view.getSize().x / static_cast<float>(window.getSize().x),
            view.getSize().y / static_cast<float>(window.getSize().y)
        };

        sf::Vector2i current_pixels = e.position;
        sf::Vector2i delta_pixels = last_drag_pixels - current_pixels;
        sf::Vector2f delta_world = {
            static_cast<float>(delta_pixels.x) * pixels_to_world.x,
            static_cast<float>(delta_pixels.y) * pixels_to_world.y
        };

        view.move(delta_world);
        window.setView(view);
        last_drag_pixels = current_pixels;
    }
}

void Game::handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e) {
    sf::View view = window.getView();
    constexpr float pan_pixels = 20.0f;
    constexpr float zoom_step = 1.05f;

    switch (e.scancode) {
        case sf::Keyboard::Scancode::W:
            view.move({0.0f, -pan_pixels});
            break;
        case sf::Keyboard::Scancode::S:
            view.move({0.0f, pan_pixels});
            break;
        case sf::Keyboard::Scancode::A:
            view.move({-pan_pixels, 0.0f});
            break;
        case sf::Keyboard::Scancode::D:
            view.move({pan_pixels, 0.0f});
            break;
        case sf::Keyboard::Scancode::Q:
            view.zoom(1.0f / zoom_step);
            break;
        case sf::Keyboard::Scancode::E:
            view.zoom(zoom_step);
            break;
        default:
            break;
    }

    window.setView(view);
}

void Game::add_circle(std::unique_ptr<EatableCircle> circle) {
    update_max_generation_from_circle(circle.get());
    circles.push_back(std::move(circle));
}

std::size_t Game::get_eater_count() const {
    std::size_t count = 0;
    for (const auto& c : circles) {
        if (dynamic_cast<EaterCircle*>(c.get())) {
            ++count;
        }
    }
    return count;
}

void Game::clear_selection() {
    selected_index.reset();
}

bool Game::select_circle_at_world(const b2Vec2& pos) {
    std::optional<std::size_t> hit;
    for (std::size_t i = 0; i < circles.size(); ++i) {
        const auto& c = circles[i];
        b2Vec2 p = c->getPosition();
        float dx = p.x - pos.x;
        float dy = p.y - pos.y;
        float dist2 = dx * dx + dy * dy;
        float r = c->getRadius();
        if (dist2 <= r * r) {
            hit = i;
        }
    }
    selected_index = hit;
    return selected_index.has_value();
}

const neat::Genome* Game::get_selected_brain() const {
    if (!selected_index || *selected_index >= circles.size()) {
        return nullptr;
    }
    if (auto* eater = dynamic_cast<EaterCircle*>(circles[*selected_index].get())) {
        return &eater->get_brain();
    }
    return nullptr;
}

const EaterCircle* Game::get_selected_eater() const {
    if (!selected_index || *selected_index >= circles.size()) {
        return nullptr;
    }
    return dynamic_cast<EaterCircle*>(circles[*selected_index].get());
}

const EaterCircle* Game::get_oldest_largest_eater() const {
    const EaterCircle* best = nullptr;
    float best_age = -1.0f;
    float best_area = -1.0f;
    for (const auto& c : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
            float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
            float area = eater->getArea();
            if (age > best_age || (std::abs(age - best_age) < 1e-6f && area > best_area)) {
                best = eater;
                best_age = age;
                best_area = area;
            }
        }
    }
    return best;
}

const EaterCircle* Game::get_oldest_smallest_eater() const {
    const EaterCircle* best = nullptr;
    float best_age = -1.0f;
    float best_area = std::numeric_limits<float>::max();
    constexpr float eps = 1e-6f;
    for (const auto& c : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
            float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
            float area = eater->getArea();
            if (age > best_age + eps || (std::abs(age - best_age) <= eps && area < best_area)) {
                best_age = age;
                best_area = area;
                best = eater;
            }
        }
    }
    return best;
}

const EaterCircle* Game::get_oldest_middle_eater() const {
    std::vector<std::pair<float, const EaterCircle*>> candidates;
    float best_age = -1.0f;
    constexpr float eps = 1e-6f;
    for (const auto& c : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
            float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
            float area = eater->getArea();
            if (age > best_age + eps) {
                best_age = age;
                candidates.clear();
                candidates.emplace_back(area, eater);
            } else if (std::abs(age - best_age) <= eps) {
                candidates.emplace_back(area, eater);
            }
        }
    }
    if (candidates.empty()) return nullptr;
    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) { return a.first < b.first; });
    return candidates[candidates.size() / 2].second;
}

const EaterCircle* Game::get_follow_target_eater() const {
    if (follow_selected) {
        if (const auto* sel = get_selected_eater()) return sel;
    }
    if (follow_oldest_largest) {
        if (const auto* e = get_oldest_largest_eater()) return e;
    }
    if (follow_oldest_smallest) {
        if (const auto* e = get_oldest_smallest_eater()) return e;
    }
    if (follow_oldest_middle) {
        if (const auto* e = get_oldest_middle_eater()) return e;
    }
    return nullptr;
}

int Game::get_selected_generation() const {
    if (!selected_index || *selected_index >= circles.size()) {
        return -1;
    }
    if (auto* eater = dynamic_cast<EaterCircle*>(circles[*selected_index].get())) {
        return eater->get_generation();
    }
    return -1;
}

void Game::update_follow_view(sf::View& view) const {
    auto center_on = [&](const EaterCircle* eater) {
        if (!eater) return false;
        b2Vec2 p = eater->getPosition();
        view.setCenter({p.x * pixles_per_meter, p.y * pixles_per_meter});
        return true;
    };

    // Priority: selected eater, then oldest-largest if enabled.
    if (follow_selected) {
        if (center_on(get_selected_eater())) {
            return;
        }
    }

    if (follow_oldest_largest) {
        const EaterCircle* best = nullptr;
        float best_age = -1.0f;
        float best_area = -1.0f;
        for (const auto& c : circles) {
            if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
                float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
                float area = eater->getArea();
                if (age > best_age || (std::abs(age - best_age) < 1e-6f && area > best_area)) {
                    best_age = age;
                    best_area = area;
                    best = eater;
                }
            }
        }
        center_on(best);
        return;
    }

    if (follow_oldest_smallest) {
        const EaterCircle* best = nullptr;
        float best_age = -1.0f;
        float best_area = std::numeric_limits<float>::max();
        for (const auto& c : circles) {
            if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
                float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
                float area = eater->getArea();
                if (age > best_age || (std::abs(age - best_age) < 1e-6f && area < best_area)) {
                    best = eater;
                    best_age = age;
                    best_area = area;
                }
            }
        }
        center_on(best);
        return;
    }

    if (follow_oldest_middle) {
        std::vector<std::pair<float, const EaterCircle*>> candidates;
        float best_age = -1.0f;
        constexpr float age_eps = 1e-6f;
        for (const auto& c : circles) {
            if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
                float age = std::max(0.0f, sim_time_accum - eater->get_creation_time());
                if (age > best_age + age_eps) {
                    best_age = age;
                    candidates.clear();
                    candidates.emplace_back(eater->getArea(), eater);
                } else if (std::abs(age - best_age) <= age_eps) {
                    candidates.emplace_back(eater->getArea(), eater);
                }
            }
        }
        if (!candidates.empty()) {
            std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) { return a.first < b.first; });
            const EaterCircle* mid = candidates[candidates.size() / 2].second;
            center_on(mid);
        }
    }
}

void Game::update_max_generation_from_circle(const EatableCircle* circle) {
    if (auto* eater = dynamic_cast<const EaterCircle*>(circle)) {
        if (eater->get_generation() > max_generation) {
            max_generation = eater->get_generation();
            max_generation_brain = eater->get_brain();
        }
    }
}

void Game::recompute_max_generation() {
    int new_max = 0;
    std::optional<neat::Genome> new_brain;
    for (const auto& circle : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(circle.get())) {
            if (eater->get_generation() >= new_max) {
                new_max = eater->get_generation();
                new_brain = eater->get_brain();
            }
        }
    }
    max_generation = new_max;
    max_generation_brain = std::move(new_brain);
}

void Game::update_max_ages() {
    float creation_max = 0.0f;
    float division_max = 0.0f;
    for (const auto& circle : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(circle.get())) {
            float age_creation = std::max(0.0f, sim_time_accum - eater->get_creation_time());
            float age_division = std::max(0.0f, sim_time_accum - eater->get_last_division_time());
            if (age_creation > creation_max) creation_max = age_creation;
            if (age_division > division_max) division_max = age_division;
        }
    }
    max_age_since_creation = creation_max;
    max_age_since_division = division_max;
}

void Game::revalidate_selection(const EatableCircle* previously_selected) {
    if (!previously_selected) {
        return;
    }
    selected_index.reset();
    for (std::size_t i = 0; i < circles.size(); ++i) {
        if (circles[i].get() == previously_selected) {
            selected_index = i;
            break;
        }
    }
}

void Game::set_selection_to_eater(const EaterCircle* eater) {
    if (!eater) {
        selected_index.reset();
        return;
    }
    for (std::size_t i = 0; i < circles.size(); ++i) {
        if (circles[i].get() == eater) {
            selected_index = i;
            return;
        }
    }
    selected_index.reset();
}

const EaterCircle* Game::find_nearest_eater(const b2Vec2& pos) const {
    const EaterCircle* best = nullptr;
    float best_dist2 = std::numeric_limits<float>::max();
    for (const auto& c : circles) {
        if (auto* eater = dynamic_cast<const EaterCircle*>(c.get())) {
            b2Vec2 p = eater->getPosition();
            float dx = p.x - pos.x;
            float dy = p.y - pos.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < best_dist2) {
                best_dist2 = d2;
                best = eater;
            }
        }
    }
    return best;
}

void Game::sprinkle_with_rate(float rate, AddType type, float dt) {
    if (rate <= 0.0f || dt <= 0.0f || petri_radius <= 0.0f) {
        return;
    }

    float expected = rate * dt;
    int guaranteed = static_cast<int>(expected);
    float remainder = expected - static_cast<float>(guaranteed);

    auto spawn_once = [&]() {
        b2Vec2 pos = random_point_in_petri();
        switch (type) {
            case AddType::Eater:
                if (auto eater = create_eater_at(pos)) {
                    update_max_generation_from_circle(eater.get());
                    circles.push_back(std::move(eater));
                }
                break;
            case AddType::Eatable:
                circles.push_back(create_eatable_at(pos, false));
                break;
            case AddType::ToxicEatable:
                circles.push_back(create_eatable_at(pos, true));
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

b2Vec2 Game::random_point_in_petri() const {
    float angle = random_unit() * 2.0f * PI;
    float radius = petri_radius * std::sqrt(random_unit());
    return b2Vec2{radius * std::cos(angle), radius * std::sin(angle)};
}

std::unique_ptr<EaterCircle> Game::create_eater_at(const b2Vec2& pos) {
    float base_area = std::max(average_eater_area, 0.0001f);
    float varied_area = base_area * (0.5f + random_unit()); // random scale around the average
    float radius = radius_from_area(varied_area);
    float angle = random_unit() * 2.0f * PI;
    const neat::Genome* base_brain = get_max_generation_brain();
    auto circle = std::make_unique<EaterCircle>(
        worldId,
        pos.x,
        pos.y,
        radius,
        circle_density,
        angle,
        0,
        init_mutation_rounds,
        init_add_node_probability,
        init_remove_node_probability,
        init_add_connection_probability,
        init_remove_connection_probability,
        base_brain,
        &neat_innovations,
        &neat_last_innov_id,
        this);
    circle->set_creation_time(sim_time_accum);
    circle->set_last_division_time(sim_time_accum);
    circle->set_impulse_magnitudes(linear_impulse_magnitude, angular_impulse_magnitude);
    circle->set_linear_damping(linear_damping, worldId);
    circle->set_angular_damping(angular_damping, worldId);
    return circle;
}

std::unique_ptr<EatableCircle> Game::create_eatable_at(const b2Vec2& pos, bool toxic) const {
    float radius = radius_from_area(add_eatable_area);
    auto circle = std::make_unique<EatableCircle>(worldId, pos.x, pos.y, radius, circle_density, toxic, 0.0f);
    circle->set_impulse_magnitudes(linear_impulse_magnitude, angular_impulse_magnitude);
    circle->set_linear_damping(linear_damping, worldId);
    circle->set_angular_damping(angular_damping, worldId);
    return circle;
}

void Game::sprinkle_entities(float dt) {
    sprinkle_with_rate(sprinkle_rate_eater, AddType::Eater, dt);
    sprinkle_with_rate(sprinkle_rate_eatable, AddType::Eatable, dt);
    sprinkle_with_rate(sprinkle_rate_toxic, AddType::ToxicEatable, dt);
}

void Game::update_eaters(const b2WorldId& worldId) {
    for (size_t i = 0; i < circles.size(); ++i) {
        if (auto* eater_circle = dynamic_cast<EaterCircle*>(circles[i].get())) {
            eater_circle->process_eating(worldId, poison_death_probability, poison_death_probability_normal);
        }
    }
}

void Game::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    const float brain_period = (brain_updates_per_sim_second > 0.0f) ? (1.0f / brain_updates_per_sim_second) : std::numeric_limits<float>::max();
    while (brain_time_accumulator >= brain_period) {
        for (size_t i = 0; i < circles.size(); ++i) {
            if (auto* eater_circle = dynamic_cast<EaterCircle*>(circles[i].get())) {
                eater_circle->set_minimum_area(minimum_area);
                eater_circle->set_use_smoothed_display(!show_true_color);
                eater_circle->move_intelligently(worldId, *this, brain_period);
            }
        }
        brain_time_accumulator -= brain_period;
    }
}

void Game::cull_consumed() {
    std::vector<std::unique_ptr<EatableCircle>> spawned_cloud;
    const EatableCircle* prev_selected = (selected_index && *selected_index < circles.size())
                                             ? circles[*selected_index].get()
                                             : nullptr;
    b2Vec2 prev_pos{0, 0};
    if (prev_selected) {
        prev_pos = prev_selected->getPosition();
    }
    bool selected_was_removed = false;
    const EaterCircle* selected_killer = nullptr;

    for (auto it = circles.begin(); it != circles.end(); ) {
        bool remove = false;

        if (auto* eater = dynamic_cast<EaterCircle*>(it->get())) {
            if (eater->is_poisoned()) {
                spawn_eatable_cloud(*eater, spawned_cloud);
                remove = true;
            } else if (eater->is_eaten()) {
                remove = true;
            }
        } else if ((*it)->is_eaten()) {
            remove = true;
        }

        if (remove) {
            if (prev_selected && prev_selected == it->get()) {
                selected_was_removed = true;
                if (auto* eater_prev = dynamic_cast<EaterCircle*>(it->get())) {
                    selected_killer = eater_prev->get_eaten_by();
                }
            }
            it = circles.erase(it);
        } else {
            ++it;
        }
    }

    if (selected_was_removed && follow_selected) {
        const EaterCircle* fallback = selected_killer;
        if (!fallback) {
            fallback = find_nearest_eater(prev_pos);
        }
        set_selection_to_eater(fallback);
    } else if (!selected_was_removed && prev_selected) {
        revalidate_selection(prev_selected);
    }
    recompute_max_generation();
    update_max_ages();

    for (auto& c : spawned_cloud) {
        circles.push_back(std::move(c));
    }
}

void Game::spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out) {
    float eater_radius = eater.getRadius();
    float total_area = eater.getArea();
    if (minimum_area <= 0.0f || total_area <= 0.0f) {
        return;
    }

    float chunk_area = std::min(minimum_area, total_area);
    float remaining_area = total_area * (std::clamp(eater_cloud_area_percentage, 0.0f, 100.0f) / 100.0f);

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

void Game::remove_outside_petri() {
    if (circles.empty()) {
        return;
    }

    const EatableCircle* prev_selected = nullptr;
    if (selected_index && *selected_index < circles.size()) {
        prev_selected = circles[*selected_index].get();
    }

    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                b2Vec2 pos = circle->getPosition();
                float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
                return distance + circle->getRadius() > petri_radius;
            }),
        circles.end());

    revalidate_selection(prev_selected);
    recompute_max_generation();
    update_max_ages();
}

void Game::remove_random_percentage(float percentage) {
    if (circles.empty() || percentage <= 0.0f) {
        return;
    }

    float clamped = std::clamp(percentage, 0.0f, 100.0f);
    std::size_t target = static_cast<std::size_t>(std::round(circles.size() * (clamped / 100.0f)));
    if (target == 0) {
        return;
    }

    std::vector<std::size_t> indices(circles.size());
    std::iota(indices.begin(), indices.end(), 0);

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);

    indices.resize(target);
    std::sort(indices.begin(), indices.end(), std::greater<std::size_t>());

    const EatableCircle* prev_selected = nullptr;
    if (selected_index && *selected_index < circles.size()) {
        prev_selected = circles[*selected_index].get();
    }

    for (std::size_t idx : indices) {
        circles.erase(circles.begin() + static_cast<std::ptrdiff_t>(idx));
    }

    revalidate_selection(prev_selected);
    recompute_max_generation();
    update_max_ages();
}

void Game::remove_stopped_boost_particles() {
    constexpr float vel_epsilon = 1e-3f;
    const EatableCircle* prev_selected = nullptr;
    if (selected_index && *selected_index < circles.size()) {
        prev_selected = circles[*selected_index].get();
    }
    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                if (!circle->is_boost_particle()) {
                    return false;
                }
                b2Vec2 v = circle->getLinearVelocity();
                return (std::fabs(v.x) <= vel_epsilon && std::fabs(v.y) <= vel_epsilon);
            }),
        circles.end());
    revalidate_selection(prev_selected);
    update_max_ages();
}

void Game::accumulate_real_time(float dt) {
    if (dt <= 0.0f) return;
    real_time_accum += dt;
    fps_accum_time += dt;
    ++fps_frames;
    if (fps_accum_time >= 0.5f) {
        fps_last = static_cast<float>(fps_frames) / fps_accum_time;
        fps_accum_time = 0.0f;
        fps_frames = 0;
    }
}

void Game::frame_rendered() {
    // reserved for any per-frame hooks; fps handled in accumulate_real_time
}

void Game::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - circle_density) < 1e-6f) {
        return;
    }
    circle_density = clamped;
    for (auto& circle : circles) {
        circle->set_density(circle_density, worldId);
    }
}

void Game::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - linear_impulse_magnitude) < 1e-6f) {
        return;
    }
    linear_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - angular_impulse_magnitude) < 1e-6f) {
        return;
    }
    angular_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::apply_impulse_magnitudes_to_circles() {
    for (auto& circle : circles) {
        circle->set_impulse_magnitudes(linear_impulse_magnitude, angular_impulse_magnitude);
    }
}

void Game::set_linear_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - linear_damping) < 1e-6f) {
        return;
    }
    linear_damping = clamped;
    apply_damping_to_circles();
}

void Game::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - angular_damping) < 1e-6f) {
        return;
    }
    angular_damping = clamped;
    apply_damping_to_circles();
}

void Game::apply_damping_to_circles() {
    for (auto& circle : circles) {
        circle->set_linear_damping(linear_damping, worldId);
        circle->set_angular_damping(angular_damping, worldId);
    }
}
