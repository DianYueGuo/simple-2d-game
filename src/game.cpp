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
    if (!b2Shape_IsValid(beginTouch.sensorShapeId) || !b2Shape_IsValid(beginTouch.visitorShapeId)) {
        return;
    }
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
    : selection(circles, timing.sim_time_accum),
      spawner(*this) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
}

Game::~Game() {
    circles.clear();
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

void Game::process_game_logic_with_speed() {
    if (paused) {
        timing.last_sim_dt = 0.0f;
        update_actual_sim_speed();
        return;
    }

    float timeStep = (1.0f / 60.0f);

    timing.desired_sim_time_accum += timeStep * timing.time_scale;

    timing.last_sim_dt = timeStep * timing.time_scale;
    update_actual_sim_speed();
    // real_time_accum should be updated by caller using frame delta; leave as is here.

    sf::Clock clock; // starts the clock
    float begin_sim_time = timing.sim_time_accum;

    while (timing.sim_time_accum + timeStep < timing.desired_sim_time_accum) {
        process_game_logic();

        if (clock.getElapsedTime() > sf::seconds(timeStep)) {
            timing.desired_sim_time_accum -= timeStep * timing.time_scale;
            timing.desired_sim_time_accum += timing.sim_time_accum - begin_sim_time;

            timing.last_sim_dt = timing.sim_time_accum - begin_sim_time;
            update_actual_sim_speed();

            break;
        }
    }
}

void Game::process_game_logic() {
    float timeStep = (1.0f / 60.0f);
    int subStepCount = 4;
    b2World_Step(worldId, timeStep, subStepCount);
    timing.sim_time_accum += timeStep;

    process_touch_events(worldId);

    brain.time_accumulator += timeStep;
    spawner.sprinkle_entities(timeStep);
    update_creatures(worldId, timeStep);
    run_brain_updates(worldId, timeStep);
    adjust_cleanup_rates();
    // continuous pellet cleanup by rate (percent per second)
    if (pellets.cleanup_rate_food > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_food * timeStep, false, false);
    }
    if (pellets.cleanup_rate_toxic > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_toxic * timeStep, true, false);
    }
    if (pellets.cleanup_rate_division > 0.0f) {
        remove_percentage_pellets(pellets.cleanup_rate_division * timeStep, false, true);
    }
    cull_consumed();
    remove_stopped_boost_particles();
    if (dish.auto_remove_outside) {
        remove_outside_petri();
    }
    update_max_ages();
    apply_selection_mode();
}

void Game::draw(sf::RenderWindow& window) const {
    // Draw petri dish boundary
    sf::CircleShape boundary(dish.radius);
    boundary.setOrigin({dish.radius, dish.radius});
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
    if (!event) {
        return;
    }

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

    if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
        handle_key_release(*keyReleased);
    }
}

sf::Vector2f Game::pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const {
    sf::Vector2f viewPos = window.mapPixelToCoords(pixel);
    return viewPos;
}

void Game::start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button) {
    view_drag.dragging = true;
    view_drag.right_dragging = is_right_button;
    view_drag.last_drag_pixels = e.position;
}

void Game::pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e) {
    if (!view_drag.dragging) {
        return;
    }

    sf::View view = window.getView();
    sf::Vector2f pixels_to_world = {
        view.getSize().x / static_cast<float>(window.getSize().x),
        view.getSize().y / static_cast<float>(window.getSize().y)
    };

    sf::Vector2i current_pixels = e.position;
    sf::Vector2i delta_pixels = view_drag.last_drag_pixels - current_pixels;
    sf::Vector2f delta_world = {
        static_cast<float>(delta_pixels.x) * pixels_to_world.x,
        static_cast<float>(delta_pixels.y) * pixels_to_world.y
    };

    view.move(delta_world);
    window.setView(view);
    view_drag.last_drag_pixels = current_pixels;
}

void Game::handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e) {
    if (e.button == sf::Mouse::Button::Left) {
        sf::Vector2f worldPos = pixel_to_world(window, e.position);

        if (cursor.mode == CursorMode::Add) {
            spawner.spawn_selected_type_at(worldPos);
            spawner.begin_add_drag_if_applicable(worldPos);
        } else if (cursor.mode == CursorMode::Select) {
            select_circle_at_world({worldPos.x, worldPos.y});
        }
    } else if (e.button == sf::Mouse::Button::Right) {
        start_view_drag(e, true);
    }
}

void Game::handle_mouse_release(const sf::Event::MouseButtonReleased& e) {
    if (e.button == sf::Mouse::Button::Right) {
        view_drag.dragging = false;
        view_drag.right_dragging = false;
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
        case sf::Keyboard::Scancode::Left:
            possesing.left_key_down = true;
            break;
        case sf::Keyboard::Scancode::Right:
            possesing.right_key_down = true;
            break;
        case sf::Keyboard::Scancode::Up:
            possesing.up_key_down = true;
            break;
        case sf::Keyboard::Scancode::Space:
            possesing.space_key_down = true;
            break;
        default:
            break;
    }

    window.setView(view);
}

void Game::handle_key_release(const sf::Event::KeyReleased& e) {
    switch (e.scancode) {
        case sf::Keyboard::Scancode::Left:
            possesing.left_key_down = false;
            break;
        case sf::Keyboard::Scancode::Right:
            possesing.right_key_down = false;
            break;
        case sf::Keyboard::Scancode::Up:
            possesing.up_key_down = false;
            break;
        case sf::Keyboard::Scancode::Space:
            possesing.space_key_down = false;
            break;
        default:
            break;
    }
}

void Game::add_circle(std::unique_ptr<EatableCircle> circle) {
    update_max_generation_from_circle(circle.get());
    adjust_pellet_count(circle.get(), 1);
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

void Game::set_selection_mode(SelectionMode mode) {
    selection_mode = mode;
    apply_selection_mode();
}

Game::SelectionMode Game::get_selection_mode() const {
    return selection_mode;
}

void Game::update_follow_view(sf::View& view) const {
    selection.update_follow_view(view);
}

void Game::apply_selection_mode() {
    switch (selection_mode) {
        case SelectionMode::OldestLargest:
            selection.set_selection_to_creature(selection.get_oldest_largest_creature());
            break;
        case SelectionMode::OldestMedian:
            selection.set_selection_to_creature(selection.get_oldest_middle_creature());
            break;
        case SelectionMode::OldestSmallest:
            selection.set_selection_to_creature(selection.get_oldest_smallest_creature());
            break;
        case SelectionMode::Manual:
        default:
            break;
    }
}

void Game::update_max_generation_from_circle(const EatableCircle* circle) {
    if (circle && circle->get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(circle);
        if (creature_circle->get_generation() > generation.max_generation) {
            generation.max_generation = creature_circle->get_generation();
            generation.brain = creature_circle->get_brain();
        }
    }
}

void Game::recompute_max_generation() {
    int new_max = 0;
    std::optional<neat::Genome> new_brain;
    for (const auto& circle : circles) {
        if (circle && circle->get_kind() == CircleKind::Creature) {
            const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
            if (creature_circle->get_generation() >= new_max) {
                new_max = creature_circle->get_generation();
                new_brain = creature_circle->get_brain();
            }
        }
    }
    generation.max_generation = new_max;
    generation.brain = std::move(new_brain);
}

void Game::update_max_ages() {
    float creation_max = 0.0f;
    float division_max = 0.0f;
    for (const auto& circle : circles) {
        if (circle && circle->get_kind() == CircleKind::Creature) {
            const auto* creature_circle = static_cast<const CreatureCircle*>(circle.get());
            float age_creation = std::max(0.0f, timing.sim_time_accum - creature_circle->get_creation_time());
            float age_division = std::max(0.0f, timing.sim_time_accum - creature_circle->get_last_division_time());
            if (age_creation > creation_max) creation_max = age_creation;
            if (age_division > division_max) division_max = age_division;
        }
    }
    age.max_age_since_creation = creation_max;
    age.max_age_since_division = division_max;
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
            creature_circle->process_eating(worldId, *this, death.poison_death_probability, death.poison_death_probability_normal);
            creature_circle->update_inactivity(dt, death.inactivity_timeout);
        }
    }
}

void Game::run_brain_updates(const b2WorldId& worldId, float timeStep) {
    (void)timeStep;
    const float brain_period = (brain.updates_per_second > 0.0f) ? (1.0f / brain.updates_per_second) : std::numeric_limits<float>::max();
    while (brain.time_accumulator >= brain_period) {
        for (size_t i = 0; i < circles.size(); ++i) {
            if (circles[i] && circles[i]->get_kind() == CircleKind::Creature) {
                auto* creature_circle = static_cast<CreatureCircle*>(circles[i].get());
                creature_circle->set_minimum_area(creature.minimum_area);
                creature_circle->set_display_mode(!show_true_color);
                creature_circle->move_intelligently(worldId, *this, brain_period);
            }
        }
        brain.time_accumulator -= brain_period;
    }
}

void Game::refresh_generation_and_age() {
    recompute_max_generation();
    update_max_ages();
}

Game::RemovalResult Game::evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud) {
    RemovalResult result{};
    if (circle.get_kind() == CircleKind::Creature) {
        const auto* creature_circle = static_cast<const CreatureCircle*>(&circle);
        if (creature_circle->is_poisoned()) {
            spawner.spawn_eatable_cloud(*creature_circle, spawned_cloud);
            result.should_remove = true;
            result.killer = creature_circle->get_eaten_by();
        } else if (creature_circle->is_eaten()) {
            result.should_remove = true;
            result.killer = creature_circle->get_eaten_by();
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
            adjust_pellet_count(it->get(), -1);
            it = circles.erase(it);
        } else {
            ++it;
        }
    }

    selection.handle_selection_after_removal(selection_snapshot, selected_was_removed, selected_killer, selection_snapshot.position);
    refresh_generation_and_age();

    for (auto& c : spawned_cloud) {
        add_circle(std::move(c));
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
            adjust_pellet_count(circles[idx].get(), -1);
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
    const float dish_radius = dish.radius;
    circles.erase(
        std::remove_if(
            circles.begin(),
            circles.end(),
            [&](const std::unique_ptr<EatableCircle>& circle) {
                b2Vec2 pos = circle->getPosition();
                float r = circle->getRadius();
                if (r >= dish_radius) {
                    if (snapshot.circle && circle.get() == snapshot.circle) {
                        selected_removed = true;
                    }
                    adjust_pellet_count(circle.get(), -1);
                    return true;
                }
                float max_center_dist = dish_radius - r;
                float dist2 = pos.x * pos.x + pos.y * pos.y;
                bool out = dist2 > max_center_dist * max_center_dist;
                if (out && snapshot.circle && circle.get() == snapshot.circle) {
                    selected_removed = true;
                }
                if (out) {
                    adjust_pellet_count(circle.get(), -1);
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
    double ratio = static_cast<double>(clamped) / 100.0;
    double available = static_cast<double>(circles.size());
    std::size_t target = static_cast<std::size_t>(std::round(available * ratio));
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

void Game::remove_percentage_pellets(float percentage, bool toxic, bool division_pellet) {
    if (circles.empty() || percentage <= 0.0f) {
        return;
    }

    std::vector<std::size_t> indices;
    indices.reserve(circles.size());
    for (std::size_t i = 0; i < circles.size(); ++i) {
        if (auto* e = dynamic_cast<const EatableCircle*>(circles[i].get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            if (e->is_toxic() == toxic && e->is_division_pellet() == division_pellet) {
                indices.push_back(i);
            }
        }
    }
    if (indices.empty()) {
        return;
    }

    float clamped = std::clamp(percentage, 0.0f, 100.0f);
    double ratio = static_cast<double>(clamped) / 100.0;
    double available = static_cast<double>(indices.size());
    std::size_t target = static_cast<std::size_t>(std::round(available * ratio));
    if (target == 0) {
        return;
    }

    static std::mt19937 rng{std::random_device{}()};
    std::shuffle(indices.begin(), indices.end(), rng);
    indices.resize(target);
    erase_indices_descending(indices);
}

std::size_t Game::count_pellets(bool toxic, bool division_pellet) const {
    std::size_t count = 0;
    const float dish_radius = dish.radius;
    for (const auto& c : circles) {
        if (auto* e = dynamic_cast<const EatableCircle*>(c.get())) {
            if (e->is_boost_particle()) continue;
            if (e->get_kind() == CircleKind::Creature) continue;
            // Only count pellets inside the petri dish.
            b2Vec2 pos = e->getPosition();
            float r = e->getRadius();
            if (r >= dish_radius) continue;
            float max_center_dist = dish_radius - r;
            float dist2 = pos.x * pos.x + pos.y * pos.y;
            if (dist2 > max_center_dist * max_center_dist) continue;
            if (e->is_toxic() == toxic && e->is_division_pellet() == division_pellet) {
                ++count;
            }
        }
    }
    return count;
}

std::size_t Game::get_cached_pellet_count(bool toxic, bool division_pellet) const {
    if (division_pellet) return pellets.division_count_cached;
    return toxic ? pellets.toxic_count_cached : pellets.food_count_cached;
}

void Game::adjust_pellet_count(const EatableCircle* circle, int delta) {
    if (!circle) return;
    if (circle->is_boost_particle()) return;
    if (circle->get_kind() == CircleKind::Creature) return;

    auto apply = [&](std::size_t& counter) {
        if (delta > 0) {
            counter += static_cast<std::size_t>(delta);
        } else {
            std::size_t dec = static_cast<std::size_t>(-delta);
            counter = (counter > dec) ? (counter - dec) : 0;
        }
    };

    if (circle->is_division_pellet()) {
        apply(pellets.division_count_cached);
    } else if (circle->is_toxic()) {
        apply(pellets.toxic_count_cached);
    } else {
        apply(pellets.food_count_cached);
    }
}

std::size_t Game::get_food_pellet_count() const {
    return pellets.food_count_cached;
}

std::size_t Game::get_toxic_pellet_count() const {
    return pellets.toxic_count_cached;
}

std::size_t Game::get_division_pellet_count() const {
    return pellets.division_count_cached;
}

void Game::adjust_cleanup_rates() {
    constexpr float PI = 3.14159f;
    float area = PI * dish.radius * dish.radius;
    float pellet_area = std::max(creature.add_eatable_area, 1e-6f);

    auto desired_count = [&](float density_target) {
        float desired_area = std::max(0.0f, density_target) * area;
        return desired_area / pellet_area;
    };

    auto compute_cleanup_rate = [&](std::size_t count, float desired) {
        if (desired <= 0.0f) {
            return count > 0 ? 100.0f : 0.0f;
        }
        float count_f = static_cast<float>(count);
        if (count_f <= desired) return 0.0f;
        float ratio = (count_f - desired) / desired;
        float rate = ratio * 50.0f; // 50%/s when double the desired
        return std::clamp(rate, 0.0f, 100.0f);
    };

    struct SpawnRates {
        float sprinkle = 0.0f;
        float cleanup = 0.0f;
    };

    auto adjust_spawn = [&](bool toxic, bool division_pellet, float density_target) -> SpawnRates {
        float desired = desired_count(density_target);
        std::size_t count = get_cached_pellet_count(toxic, division_pellet);
        float diff = desired - static_cast<float>(count);
        float sprinkle_rate = (diff > 0.0f) ? std::min(diff * 0.5f, 200.0f) : 0.0f;
        float cleanup_rate = compute_cleanup_rate(count, desired);
        return {sprinkle_rate, cleanup_rate};
    };

    auto food_rates = adjust_spawn(false, false, pellets.food_density);
    auto toxic_rates = adjust_spawn(true, false, pellets.toxic_density);
    auto division_rates = adjust_spawn(false, true, pellets.division_density);
    pellets.sprinkle_rate_eatable = food_rates.sprinkle;
    pellets.cleanup_rate_food = food_rates.cleanup;
    pellets.sprinkle_rate_toxic = toxic_rates.sprinkle;
    pellets.cleanup_rate_toxic = toxic_rates.cleanup;
    pellets.sprinkle_rate_division = division_rates.sprinkle;
    pellets.cleanup_rate_division = division_rates.cleanup;
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
    timing.last_real_dt = dt;
    timing.real_time_accum += dt;
    fps.accum_time += dt;
    ++fps.frames;
    if (fps.accum_time >= 0.5f) {
        fps.last = static_cast<float>(fps.frames) / fps.accum_time;
        fps.accum_time = 0.0f;
        fps.frames = 0;
    }
}

void Game::frame_rendered() {
    // reserved for any per-frame hooks; fps handled in accumulate_real_time
}

void Game::update_actual_sim_speed() {
    constexpr float eps = std::numeric_limits<float>::epsilon();
    if (timing.last_real_dt > eps) {
        timing.actual_sim_speed_inst = timing.last_sim_dt / timing.last_real_dt;
    } else {
        timing.actual_sim_speed_inst = 0.0f;
    }
}

void Game::set_circle_density(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.circle_density) < 1e-6f) {
        return;
    }
    movement.circle_density = clamped;
    for (auto& circle : circles) {
        circle->set_density(movement.circle_density, worldId);
    }
}

void Game::set_linear_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - movement.linear_impulse_magnitude) < 1e-6f) {
        return;
    }
    movement.linear_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::set_angular_impulse_magnitude(float m) {
    float clamped = std::max(m, 0.0f);
    if (std::abs(clamped - movement.angular_impulse_magnitude) < 1e-6f) {
        return;
    }
    movement.angular_impulse_magnitude = clamped;
    apply_impulse_magnitudes_to_circles();
}

void Game::apply_impulse_magnitudes_to_circles() {
    for (auto& circle : circles) {
        circle->set_impulse_magnitudes(movement.linear_impulse_magnitude, movement.angular_impulse_magnitude);
    }
}

void Game::set_linear_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.linear_damping) < 1e-6f) {
        return;
    }
    movement.linear_damping = clamped;
    apply_damping_to_circles();
}

void Game::set_angular_damping(float d) {
    float clamped = std::max(d, 0.0f);
    if (std::abs(clamped - movement.angular_damping) < 1e-6f) {
        return;
    }
    movement.angular_damping = clamped;
    apply_damping_to_circles();
}

void Game::apply_damping_to_circles() {
    for (auto& circle : circles) {
        circle->set_linear_damping(movement.linear_damping, worldId);
        circle->set_angular_damping(movement.angular_damping, worldId);
    }
}
