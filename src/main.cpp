#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "circle_physics.hpp"
#include "eater_circle.hpp"

#include <time.h>


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

int main() {
    srand(time(NULL));

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    std::vector<std::unique_ptr<EaterCircle>> circles;

    circles.push_back(
        std::make_unique<EaterCircle>(
                        worldId,
                        100.0f,
                        100.0f,
                        10.0f,
                        1.0f,
                        0.3f
        )
    );

    float timeStep = 1.0f / 60.0f;
    int subStepCount = 4;

    sf::RenderWindow window(sf::VideoMode({640, 480}), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        b2World_Step(worldId, timeStep, subStepCount);

        process_touch_events(worldId);

        for (auto& circle : circles) {
            if (auto* eater_circle = dynamic_cast<EaterCircle*>(circle.get())) {
                eater_circle->process_eating();
            }
        }

        for (auto it = circles.begin(); it != circles.end(); ) {
            if ((*it)->is_eaten()) {
                it = circles.erase(it);
            } else {
                ++it;
            }
        }

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (ImGui::GetIO().WantCaptureMouse)
                continue;

            if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    circles.push_back(
                    std::make_unique<EaterCircle>(
                        worldId,
                        mouseButtonPressed->position.x,
                        mouseButtonPressed->position.y,
                        10.0f * (0.5f + static_cast<float>(rand()) / RAND_MAX),
                        1.0f,
                        0.3f
                    )
                    );
                }
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
                    circles.at(0)->apply_forward_force();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
                    circles.at(0)->stop_applying_force();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Left)
                    circles.at(0)->apply_left_turn_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Right)
                    circles.at(0)->apply_right_turn_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Left)
                    circles.at(0)->stop_applying_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Right)
                    circles.at(0)->stop_applying_torque();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();

        for (const auto& circle : circles) {
            circle->draw(window);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    b2DestroyWorld(worldId);

    return 0;
}
