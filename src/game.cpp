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
        // process begin event
        std::cout << "Begin touch event detected!" << std::endl;
        std::cout << "Visitor radius: " << static_cast<CirclePhysics*>(visitorShapeUserData)->getRadius() << std::endl;

        static_cast<CirclePhysics*>(sensorShapeUserData)->add_touching_circle(static_cast<CirclePhysics*>(visitorShapeUserData));
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        if (b2Shape_IsValid(endTouch->sensorShapeId) && b2Shape_IsValid(endTouch->visitorShapeId))
        {
            void* sensorShapeUserData = b2Shape_GetUserData(endTouch->sensorShapeId);
            void* visitorShapeUserData = b2Shape_GetUserData(endTouch->visitorShapeId);
            // process end event
            std::cout << "End touch event detected!" << std::endl;
            std::cout << "Visitor radius: " << static_cast<CirclePhysics*>(visitorShapeUserData)->getRadius() << std::endl;

            static_cast<CirclePhysics*>(sensorShapeUserData)->remove_touching_circle(static_cast<CirclePhysics*>(visitorShapeUserData));
        }
    }
}

void Game::process_game_logic() {
    float timeStep = 1.0f / 60.0f;
    int subStepCount = 4;
    b2World_Step(worldId, timeStep, subStepCount);

    process_touch_events(worldId);

    for (auto& circle : circles) {
        if (auto* eater_circle = dynamic_cast<EaterCircle*>(circle.get())) {
            eater_circle->process_eating(worldId);
        }
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
    }
}
