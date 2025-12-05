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
            eater_circle->process_eating(worldId);
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
            sf::Vector2f worldPos = window.mapPixelToCoords(mouseButtonPressed->position);

            circles.push_back(
            std::make_unique<EaterCircle>(
                worldId,
                worldPos.x / pixles_per_meter,
                worldPos.y / pixles_per_meter,
                1.0f * (0.5f + static_cast<float>(rand()) / RAND_MAX),
                1.0f,
                0.3f
            )
            );
        }
    }

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        float probability = static_cast<float>(rand()) / RAND_MAX;

        if (keyPressed->scancode == sf::Keyboard::Scancode::Space && probability > 0.8f)
            circles.at(0)->apply_forward_impulse();

        if (keyPressed->scancode == sf::Keyboard::Scancode::Left && probability > 0.8f)
            circles.at(0)->apply_left_turn_impulse();

        if (keyPressed->scancode == sf::Keyboard::Scancode::Right && probability > 0.8f)
            circles.at(0)->apply_right_turn_impulse();

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
