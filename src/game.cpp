#include <iostream>

#include "game.hpp"

Game::Game() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);

    circles.push_back(
        std::make_unique<EaterCircle>(
                        worldId,
                        10.0f,
                        10.0f,
                        1.0f,
                        1.0f,
                        0.3f
        )
    );
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

    // Use index-based iteration so push_back inside eater logic doesn't invalidate references
    for (size_t i = 0; i < circles.size(); ++i) {
        if (auto* eater_circle = dynamic_cast<EaterCircle*>(circles[i].get())) {
            eater_circle->process_eating(worldId, poison_death_probability);
        }
    }

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

    for (auto it = circles.begin(); it != circles.end(); ) {
        if ((*it)->is_eaten()) {
            it = circles.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::draw(sf::RenderWindow& window) const {
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
                        circles.push_back(
                            std::make_unique<EaterCircle>(
                                worldId,
                                pos.x,
                                pos.y,
                                1.0f * (0.5f + static_cast<float>(rand()) / RAND_MAX),
                                1.0f,
                                0.3f
                            )
                        );
                        break;
                    case AddType::Eatable:
                        circles.push_back(
                            std::make_unique<EatableCircle>(
                                worldId,
                                pos.x,
                                pos.y,
                                std::sqrt(add_eatable_area / 3.14159f),
                                1.0f,
                                0.3f,
                                false
                            )
                        );
                        break;
                    case AddType::ToxicEatable:
                        circles.push_back(
                            std::make_unique<EatableCircle>(
                                worldId,
                                pos.x,
                                pos.y,
                                std::sqrt(add_eatable_area / 3.14159f),
                                1.0f,
                                0.3f,
                                true
                            )
                        );
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
