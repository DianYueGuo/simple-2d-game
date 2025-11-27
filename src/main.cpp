#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "circle_physics.hpp"

#include <time.h>


void draw_circle(sf::RenderWindow& window, const CirclePhysics& circle_physics) {
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    shape.setOrigin({circle_physics.getRadius(), circle_physics.getRadius()});
    shape.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});
    shape.setRadius(circle_physics.getRadius());
    window.draw(shape);

    sf::RectangleShape line({circle_physics.getRadius(), circle_physics.getRadius() / 4.0f});
    line.setFillColor(sf::Color::White);
    line.rotate(sf::radians(circle_physics.getAngle()));

    line.setOrigin({0, circle_physics.getRadius() / 4.0f / 2.0f});
    line.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});

    window.draw(line);
}

void process_touch_events(const b2WorldId& worldId) {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        b2SensorBeginTouchEvent* beginTouch = sensorEvents.beginEvents + i;
        void* myUserData = b2Shape_GetUserData(beginTouch->visitorShapeId);
        // process begin event
        std::cout << "Begin touch event detected!" << std::endl;
    }

    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        b2SensorEndTouchEvent* endTouch = sensorEvents.endEvents + i;
        if (b2Shape_IsValid(endTouch->visitorShapeId))
        {
            void* myUserData = b2Shape_GetUserData(endTouch->visitorShapeId);
            // process end event
            std::cout << "End touch event detected!" << std::endl;
        }
    }
}

int main() {
    srand(time(NULL));

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    std::vector<CirclePhysics> circle_physics;

    circle_physics.emplace_back(
                        worldId,
                        100.0f,
                        100.0f,
                        10.0f,
                        1.0f,
                        0.3f
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
                    circle_physics.emplace_back(
                        worldId,
                        mouseButtonPressed->position.x,
                        mouseButtonPressed->position.y,
                        10.0f * (0.5f + static_cast<float>(rand()) / RAND_MAX),
                        1.0f,
                        0.3f
                    );
                }
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
                    circle_physics.at(0).apply_forward_force();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Space)
                    circle_physics.at(0).stop_applying_force();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Left)
                    circle_physics.at(0).apply_left_turn_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Right)
                    circle_physics.at(0).apply_right_turn_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Left)
                    circle_physics.at(0).stop_applying_torque();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyReleased>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Right)
                    circle_physics.at(0).stop_applying_torque();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();

        for (const auto& circle_physics : circle_physics) {
            draw_circle(window, circle_physics);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    b2DestroyWorld(worldId);

    return 0;
}
