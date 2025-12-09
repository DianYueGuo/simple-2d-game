#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>
#include <random>

#include "game.hpp"
#include "creature_circle.hpp"

namespace {
CirclePhysics* circle_from_shape(const b2ShapeId& shapeId) {
    return static_cast<CirclePhysics*>(b2Shape_GetUserData(shapeId));
}

void handle_sensor_begin_touch(const b2SensorBeginTouchEvent& beginTouch) {
    if (auto* sensor = circle_from_shape(beginTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(beginTouch.visitorShapeId)) {
            sensor->add_touching_circle(visitor);
        }
    }
}

void handle_sensor_end_touch(const b2SensorEndTouchEvent& endTouch) {
    if (!b2Shape_IsValid(endTouch.sensorShapeId) || !b2Shape_IsValid(endTouch.visitorShapeId)) {
        return;
    }
    if (auto* sensor = circle_from_shape(endTouch.sensorShapeId)) {
        if (auto* visitor = circle_from_shape(endTouch.visitorShapeId)) {
            sensor->remove_touching_circle(visitor);
        }
    }
}
} // namespace

Game::Game()
    : selection(circles, sim_time_accum),
      spawner(*this) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{0.0f, 0.0f};
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
        handle_sensor_begin_touch(*beginTouch);
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        handle_sensor_end_touch(*endTouch);
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
    spawner.sprinkle_entities(timeStep);
    update_creatures(worldId, timeStep);
    run_brain_updates(worldId, timeStep);
    adjust_cleanup_rates();
    // continuous pellet cleanup by rate (percent per second)
    if (cleanup_rate_food > 0.0f) {
        remove_percentage_pellets(cleanup_rate_food * timeStep, false, false);
    }
    if (cleanup_rate_toxic > 0.0f) {
        remove_percentage_pellets(cleanup_rate_toxic * timeStep, true, false);
    }
    if (cleanup_rate_division > 0.0f) {
        remove_percentage_pellets(cleanup_rate_division * timeStep, false, true);
    }
    cull_consumed();
    remove_stopped_boost_particles();
    if (auto_remove_outside) {
        remove_outside_petri();
    }
    update_max_ages();
}

void Game::draw(sf::RenderWindow& window) const {
    // Draw petri dish boundary
    sf::CircleShape boundary(petri_radius);
    boundary.setOrigin({petri_radius, petri_radius});
    boundary.setPosition({0.0f, 0.0f});
    boundary.setOutlineColor(sf::Color::White);
    boundary.setOutlineThickness(0.2f);
    boundary.setFillColor(sf::Color::Transparent);
    window.draw(boundary);

    for (const auto& circle : circles) {
        circle->draw(window);
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

sf::Vector2f Game::pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const {
    sf::Vector2f viewPos = window.mapPixelToCoords(pixel);
    return viewPos;
}

void Game::start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button) {
    dragging = true;
    right_dragging = is_right_button;
    last_drag_pixels = e.position;
}

void Game::pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    if (!dragging) {
        return;
    }

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

void Game::handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e) {
    if (e.button == sf::Mouse::Button::Left) {
        sf::Vector2f worldPos = pixel_to_world(window, e.position);

        if (cursor_mode == CursorMode::Add) {
            spawner.try_add_circle_at(worldPos);
            spawner.begin_add_drag_if_applicable(worldPos);
        } else if (cursor_mode == CursorMode::Select) {
            select_circle_at_world({worldPos.x, worldPos.y});
        }
    } else if (e.button == sf::Mouse::Button::Right) {
        start_view_drag(e, true);
    }
}

void Game::handle_mouse_release(const sf::Event::MouseButtonReleased& e) {
    if (e.button == sf::Mouse::Button::Right) {
        dragging = false;
        right_dragging = false;
    }
    if (e.button == sf::Mouse::Button::Left) {
        spawner.reset_add_drag_state();
    }
}

void Game::handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    spawner.continue_add_drag(pixel_to_world(window, {e.position.x, e.position.y}));
    pan_view(window, e);
}

void Game::handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e) {
    sf::View view = window.getView();
    const float pan_fraction = 0.02f;
    const float pan_x = view.getSize().x * pan_fraction;
    const float pan_y = view.getSize().y * pan_fraction;
    constexpr float zoom_step = 1.05f;

    switch (e.scancode) {
        case sf::Keyboard::Scancode::W:
            view.move({0.0f, -pan_y});
            break;
        case sf::Keyboard::Scancode::S:
            view.move({0.0f, pan_y});
            break;
        case sf::Keyboard::Scancode::A:
            view.move({-pan_x, 0.0f});
            break;
        case sf::Keyboard::Scancode::D:
            view.move({pan_x, 0.0f});
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

std::size_t Game::get_creature_count() const {
    std::size_t count = 0;
    for (const auto& c : circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            ++count;
        }
    }
    return count;
}

void Game::clear_selection() {
    selection.clear();
}

bool Game::select_circle_at_world(const b2Vec2& pos) {
    return selection.select_circle_at_world(pos);
}

const neat::Genome* Game::get_selected_brain() const {
    return selection.get_selected_brain();
}

const CreatureCircle* Game::get_selected_creature() const {
    return selection.get_selected_creature();
}

const CreatureCircle* Game::get_oldest_largest_creature() const {
    return selection.get_oldest_largest_creature();
}

const CreatureCircle* Game::get_oldest_smallest_creature() const {
    return selection.get_oldest_smallest_creature();
}

const CreatureCircle* Game::get_oldest_middle_creature() const {
    return selection.get_oldest_middle_creature();
}

const CreatureCircle* Game::get_follow_target_creature() const {
    return selection.get_follow_target_creature();
}

int Game::get_selected_generation() const {
    return selection.get_selected_generation();
}

void Game::set_follow_selected(bool v) {
    selection.set_follow_selected(v);
}

bool Game::get_follow_selected() const {
    return selection.get_follow_selected();
}

void Game::set_follow_oldest_largest(bool v) {
    selection.set_follow_oldest_largest(v);
}

bool Game::get_follow_oldest_largest() const {
    return selection.get_follow_oldest_largest();
}

void Game::set_follow_oldest_smallest(bool v) {
    selection.set_follow_oldest_smallest(v);
}

bool Game::get_follow_oldest_smallest() const {
    return selection.get_follow_oldest_smallest();
}

void Game::set_follow_oldest_middle(bool v) {
    selection.set_follow_oldest_middle(v);
}

bool Game::get_follow_oldest_middle() const {
    return selection.get_follow_oldest_middle();
}

void Game::update_follow_view(sf::View& view) const {
    selection.update_follow_view(view);
}

void Game::update_max_generation_from_circle(const EatableCircle* circle) {
    if (circle && circle->get_kind() == CircleKind::Creature) {
        auto* creature = static_cast<const CreatureCircle*>(circle);
        if (creature->get_generation() > max_generation) {
            max_generation = creature->get_generation();
            max_generation_brain = creature->get_brain();
        }
    }
}

void Game::recompute_max_generation() {
    int new_max = 0;
    std::optional<neat::Genome> new_brain;
    for (const auto& circle : circles) {
        if (circle && circle->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(circle.get());
            if (creature->get_generation() >= new_max) {
                new_max = creature->get_generation();
                new_brain = creature->get_brain();
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
        if (circle && circle->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(circle.get());
            float age_creation = std::max(0.0f, sim_time_accum - creature->get_creation_time());
            float age_division = std::max(0.0f, sim_time_accum - creature->get_last_division_time());
            if (age_creation > creation_max) creation_max = age_creation;
            if (age_division > division_max) division_max = age_division;
        }
    }
    max_age_since_creation = creation_max;
    max_age_since_division = division_max;
}

void Game::set_selection_to_creature(const CreatureCircle* creature) {
    selection.set_selection_to_creature(creature);
}

const CreatureCircle* Game::find_nearest_creature(const b2Vec2& pos) const {
    return selection.find_nearest_creature(pos);
}

void Game::update_creatures(const b2WorldId& worldId, float dt) {
    for (size_t i = 0; i < circles.size(); ++i) {
        if (circles[i] && circles[i]->get_kind() == CircleKind::Creature) {
            auto* creature_circle = static_cast<CreatureCircle*>(circles[i].get());
            creature_circle->process_eating(worldId, *this, poison_death_probability, poison_death_probability_normal);
            creature_circle->update_inactivity(dt, inactivity_timeout);
        }
    }
}

void Game::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    (void)timeStep;
    const float brain_period = (brain_updates_per_sim_second > 0.0f) ? (1.0f / brain_updates_per_sim_second) : std::numeric_limits<float>::max();
    while (brain_time_accumulator >= brain_period) {
        for (size_t i = 0; i < circles.size(); ++i) {
            if (circles[i] && circles[i]->get_kind() == CircleKind::Creature) {
                auto* creature_circle = static_cast<CreatureCircle*>(circles[i].get());
                creature_circle->set_minimum_area(minimum_area);
                creature_circle->set_display_mode(!show_true_color);
                creature_circle->move_intelligently(worldId, *this, brain_period);
            }
        }
        brain_time_accumulator -= brain_period;
    }
}

void Game::refresh_generation_and_age() {
    recompute_max_generation();
    update_max_ages();
}

Game::RemovalResult Game::evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    RemovalResult result{};
    if (circle.get_kind() == CircleKind::Creature) {
        auto* creature = static_cast<CreatureCircle*>(&circle);
        if (creature->is_poisoned()) {
            spawner.spawn_eatable_cloud(*creature, spawned_cloud);
            result.should_remove = true;
            result.killer = creature->get_eaten_by();
        } else if (creature->is_eaten()) {
            result.should_remove = true;
            result.killer = creature->get_eaten_by();
        }
    } else if (circle.is_eaten()) {
        result.should_remove = true;
    }
    return result;
}

void Game::cull_consumed() {
    std::vector<std::unique_ptr<EatableCircle>> spawned_cloud;
    auto selection_snapshot = selection.capture_snapshot();
    bool selected_was_removed = false;
    const CreatureCircle* selected_killer = nullptr;

    for (auto it = circles.begin(); it != circles.end(); ) {
        RemovalResult removal = evaluate_circle_removal(**it, spawned_cloud);
        if (removal.should_remove) {
            if (selection_snapshot.circle && selection_snapshot.circle == it->get()) {
                selected_was_removed = true;
                selected_killer = removal.killer;
            }
            it = circles.erase(it);
        } else {
            ++it;
        }
    }

    selection.handle_selection_after_removal(selection_snapshot, selected_was_removed, selected_killer, selection_snapshot.position);
    refresh_generation_and_age();

    for (auto& c : spawned_cloud) {
        circles.push_back(std::move(c));
    }
}

void Game::erase_indices_descending(std::vector<std::size_t>& indices) {
    if (indices.empty()) {
        return;
    }

    auto snapshot = selection.capture_snapshot();

    std::sort(indices.begin(), indices.end(), std::greater<std::size_t>());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    for (std::size_t idx : indices) {
        if (idx < circles.size()) {
            circles.erase(circles.begin() + static_cast<std::ptrdiff_t>(idx));
        }
    }

    selection.revalidate_selection(snapshot.circle);
    refresh_generation_and_age();
}

void Game::remove_outside_petri() {
    if (circles.empty()) {
        return;
    }

    auto snapshot = selection.capture_snapshot();

    bool selected_removed = false;
    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                b2Vec2 pos = circle->getPosition();
                float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
                bool out = distance + circle->getRadius() > petri_radius;
                if (out && snapshot.circle && circle.get() == snapshot.circle) {
                    selected_removed = true;
                }
                return out;
            }),
        circles.end());

    selection.handle_selection_after_removal(snapshot, selected_removed, nullptr, snapshot.position);
    refresh_generation_and_age();
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
    erase_indices_descending(indices);
}

void Game::remove_percentage_pellets(float percentage, bool toxic, bool division_boost) {
    if (circles.empty() || percentage <= 0.0f) {
        return;
    }

    std::vector<std::size_t> indices;
    indices.reserve(circles.size());
    for (std::size_t i = 0; i < circles.size(); ++i) {
        if (auto* e = dynamic_cast<EatableCircle*>(circles[i].get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            if (e->is_toxic() == toxic && e->is_division_boost() == division_boost) {
                indices.push_back(i);
            }
        }
    }
    if (indices.empty()) {
        return;
    }

    float clamped = std::clamp(percentage, 0.0f, 100.0f);
    std::size_t target = static_cast<std::size_t>(std::round(indices.size() * (clamped / 100.0f)));
    if (target == 0) {
        return;
    }

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(target);
    erase_indices_descending(indices);
}

std::size_t Game::count_pellets(bool toxic, bool division_boost) const {
    std::size_t count = 0;
    for (const auto& c : circles) {
        if (auto* e = dynamic_cast<const EatableCircle*>(c.get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            // Only count pellets inside the petri dish.
            b2Vec2 pos = e->getPosition();
            float dist = std::sqrt(pos.x * pos.x + pos.y * pos.y);
            if (dist + e->getRadius() > petri_radius) continue;
            if (e->is_toxic() == toxic && e->is_division_boost() == division_boost) {
                ++count;
            }
        }
    }
    return count;
}

std::size_t Game::get_food_pellet_count() const {
    return count_pellets(false, false);
}

std::size_t Game::get_toxic_pellet_count() const {
    return count_pellets(true, false);
}

std::size_t Game::get_division_pellet_count() const {
    return count_pellets(false, true);
}

void Game::adjust_cleanup_rates() {
    constexpr float PI = 3.14159f;
    float area = PI * petri_radius * petri_radius;
    float pellet_area = std::max(add_eatable_area, 1e-6f);

    auto desired_count = [&](float density_target) {
        float desired_area = std::max(0.0f, density_target) * area;
        return desired_area / pellet_area;
    };

    auto compute_cleanup_rate = [&](std::size_t count, float desired) {
        if (desired <= 0.0f) {
            return count > 0 ? 100.0f : 0.0f;
        }
        if (count <= desired) return 0.0f;
        float ratio = (static_cast<float>(count) - desired) / desired;
        float rate = ratio * 50.0f; // 50%/s when double the desired
        return std::clamp(rate, 0.0f, 100.0f);
    };

    auto adjust_spawn = [&](bool toxic, bool division_boost, float density_target, float& sprinkle_rate_out, float& cleanup_rate_out) {
        float desired = desired_count(density_target);
        std::size_t count = count_pellets(toxic, division_boost);
        float diff = desired - static_cast<float>(count);
        if (diff > 0.0f) {
            // Need to add
            sprinkle_rate_out = std::min(diff * 0.5f, 200.0f);
        } else {
            sprinkle_rate_out = 0.0f;
        }
        cleanup_rate_out = compute_cleanup_rate(count, desired);
    };

    adjust_spawn(false, false, food_pellet_density, sprinkle_rate_eatable, cleanup_rate_food);
    adjust_spawn(true, false, toxic_pellet_density, sprinkle_rate_toxic, cleanup_rate_toxic);
    adjust_spawn(false, true, division_pellet_density, sprinkle_rate_division, cleanup_rate_division);
}

void Game::remove_stopped_boost_particles() {
    constexpr float vel_epsilon = 1e-3f;
    auto snapshot = selection.capture_snapshot();
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
    selection.handle_selection_after_removal(snapshot, false, nullptr, snapshot.position);
    refresh_generation_and_age();
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
