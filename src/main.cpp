#include <iostream>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>


class CirclePhysics {
public:
    CirclePhysics(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f) {
        b2BodyDef circleBodyDef = b2DefaultBodyDef();
        circleBodyDef.type = b2_dynamicBody;
        circleBodyDef.position = (b2Vec2){position_x, position_y};

        bodyId = b2CreateBody(worldId, &circleBodyDef);

        b2ShapeDef CircleShapeDef = b2DefaultShapeDef();
        CircleShapeDef.density = density;
        CircleShapeDef.material.friction = friction;

        b2Circle circle;
        circle.center = (b2Vec2){0.0f, 0.0f};
        circle.radius = radius;

        b2CreateCircleShape(bodyId, &CircleShapeDef, &circle);
    }

    b2Vec2 getPosition() {
        return b2Body_GetPosition(bodyId);
    }

    float getRadius() {
        b2ShapeId shapeId;
        b2Body_GetShapes(bodyId, &shapeId, 1);
        b2Circle circle = b2Shape_GetCircle(shapeId);
        return circle.radius;
    }
private:
    b2BodyId bodyId;
};

int main() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    CirclePhysics circle_physics(worldId, 10.0f, 10.0f, 10.0f);

    float timeStep = 1.0f / 60.0f;
    int subStepCount = 4;

    sf::RenderWindow window(sf::VideoMode({640, 480}), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        b2World_Step(worldId, timeStep, subStepCount);

        shape.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});
        shape.setRadius(circle_physics.getRadius());

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if ( ImGui::GetIO().WantCaptureMouse ){
		            std::cout << "ImGui captured the mouse click" << std::endl;
	            }

                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    std::cout << "the left button was pressed" << std::endl;
                    std::cout << "mouse x: " << mouseButtonPressed->position.x << std::endl;
                    std::cout << "mouse y: " << mouseButtonPressed->position.y << std::endl;
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();
        window.draw(shape);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    b2DestroyWorld(worldId);

    return 0;
}
