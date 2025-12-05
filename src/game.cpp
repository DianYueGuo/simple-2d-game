#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <limits>

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

    circles.push_back(create_eater_at({0.0f, 0.0f}));
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
    float timeStep = (1.0f / 60.0f) * time_scale;
    int subStepCount = 4;
    b2World_Step(worldId, timeStep, subStepCount);

    process_touch_events(worldId);

    brain_time_accumulator += timeStep;
    sprinkle_entities(timeStep);
    update_eaters(worldId);
    run_brain_updates(worldId, timeStep);
    cull_consumed();
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
        if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
            sf::Vector2f viewPos = window.mapPixelToCoords(mouseButtonPressed->position);
            sf::Vector2f worldPos = {viewPos.x / pixles_per_meter, viewPos.y / pixles_per_meter};

            auto add_circle_at = [&](sf::Vector2f pos) {
                switch (add_type) {
                    case AddType::Eater:
                        circles.push_back(create_eater_at({pos.x, pos.y}));
                        break;
                    case AddType::Eatable:
                        circles.push_back(create_eatable_at({pos.x, pos.y}, false));
                        break;
                    case AddType::ToxicEatable:
                        circles.push_back(create_eatable_at({pos.x, pos.y}, true));
                        break;
                }
            };

            switch (cursor_mode) {
                case CursorMode::Add: {
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
                    break;
                }
                case CursorMode::Drag: {
                    dragging = true;
                    right_dragging = false;
                    last_drag_pixels = mouseButtonPressed->position;
                    break;
                }
            }
        } else if (mouseButtonPressed->button == sf::Mouse::Button::Right) {
            dragging = true;
            right_dragging = true;
            last_drag_pixels = mouseButtonPressed->position;
        }
    }

    if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if ((mouseButtonReleased->button == sf::Mouse::Button::Left && cursor_mode == CursorMode::Drag) ||
            mouseButtonReleased->button == sf::Mouse::Button::Right) {
            dragging = false;
            right_dragging = false;
        }
        if (mouseButtonReleased->button == sf::Mouse::Button::Left) {
            add_dragging = false;
            last_add_world_pos.reset();
            last_drag_world_pos.reset();
            add_drag_distance = 0.0f;
        }
    }

    if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        if (add_dragging && cursor_mode == CursorMode::Add) {
            sf::Vector2f viewPos = window.mapPixelToCoords({mouseMoved->position.x, mouseMoved->position.y});
            sf::Vector2f worldPos = {viewPos.x / pixles_per_meter, viewPos.y / pixles_per_meter};

            if (!last_drag_world_pos) {
                last_drag_world_pos = worldPos;
            }

            float dx_move = worldPos.x - last_drag_world_pos->x;
            float dy_move = worldPos.y - last_drag_world_pos->y;
            add_drag_distance += std::sqrt(dx_move * dx_move + dy_move * dy_move);
            last_drag_world_pos = worldPos;

            const float min_spacing = std::sqrt(add_eatable_area / 3.14159f) * 2.0f;

            if (add_drag_distance >= min_spacing) {
                switch (add_type) {
                    case AddType::Eater:
                        break;
                    case AddType::Eatable:
                        circles.push_back(
                            std::make_unique<EatableCircle>(
                                worldId,
                                worldPos.x,
                                worldPos.y,
                                std::sqrt(add_eatable_area / 3.14159f),
                                1.0f,
                                0.3f,
                                false
                            )
                        );
                        last_add_world_pos = worldPos;
                        break;
                    case AddType::ToxicEatable:
                        circles.push_back(
                            std::make_unique<EatableCircle>(
                                worldId,
                                worldPos.x,
                                worldPos.y,
                                std::sqrt(add_eatable_area / 3.14159f),
                                1.0f,
                                0.3f,
                                true
                            )
                        );
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

            sf::Vector2i current_pixels = mouseMoved->position;
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
        sf::View view = window.getView();
        constexpr float pan_pixels = 20.0f;
        constexpr float zoom_step = 1.05f;

        switch (keyPressed->scancode) {
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
}

void Game::add_circle(std::unique_ptr<EatableCircle> circle) {
    circles.push_back(std::move(circle));
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
                circles.push_back(create_eater_at(pos));
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

std::unique_ptr<EaterCircle> Game::create_eater_at(const b2Vec2& pos) const {
    float radius = 1.0f * (0.5f + random_unit());
    return std::make_unique<EaterCircle>(worldId, pos.x, pos.y, radius, 1.0f, 0.3f);
}

std::unique_ptr<EatableCircle> Game::create_eatable_at(const b2Vec2& pos, bool toxic) const {
    float radius = radius_from_area(add_eatable_area);
    return std::make_unique<EatableCircle>(worldId, pos.x, pos.y, radius, 1.0f, 0.3f, toxic);
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
                eater_circle->move_intelligently(worldId, *this);
            }
        }
        brain_time_accumulator -= brain_period;
    }
}

void Game::cull_consumed() {
    std::vector<std::unique_ptr<EatableCircle>> spawned_cloud;

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
            it = circles.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& c : spawned_cloud) {
        circles.push_back(std::move(c));
    }
}

void Game::spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out) {
    float eater_radius = eater.getRadius();
    float total_area = PI * eater_radius * eater_radius;
    if (minimum_area <= 0.0f || total_area <= 0.0f) {
        return;
    }

    float chunk_area = std::min(minimum_area, total_area);
    float remaining_area = total_area;

    while (remaining_area > 0.0f) {
        float use_area = std::min(chunk_area, remaining_area);
        float piece_radius = radius_from_area(use_area);
        float max_offset = std::max(0.0f, eater_radius - piece_radius);

        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
        float dist = max_offset * std::sqrt(static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
        b2Vec2 pos = eater.getPosition();
        float x = pos.x + std::cos(angle) * dist;
        float y = pos.y + std::sin(angle) * dist;

        auto piece = std::make_unique<EatableCircle>(
            worldId,
            x,
            y,
            piece_radius,
            1.0f,
            0.3f,
            false
        );
        out.push_back(std::move(piece));

        remaining_area -= use_area;
    }
}
