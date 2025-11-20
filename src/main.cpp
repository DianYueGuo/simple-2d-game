#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>


int main() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    b2WorldId worldId = b2CreateWorld(&worldDef);

    b2BodyDef circleBodyDef = b2DefaultBodyDef();
    circleBodyDef.type = b2_dynamicBody;
    circleBodyDef.position = (b2Vec2){10.0f, 10.0f};
    b2BodyId circleId = b2CreateBody(worldId, &circleBodyDef);
    b2ShapeDef CircleShapeDef = b2DefaultShapeDef();
    CircleShapeDef.density = 1.0f;
    CircleShapeDef.material.friction = 0.3f;
    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = 10.0f;
    b2CreateCircleShape(circleId, &CircleShapeDef, &circle);

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

        b2Vec2 position = b2Body_GetPosition(circleId);
        shape.setPosition({position.x, position.y});
        b2ShapeId shapeId;
        b2Body_GetShapes(circleId, &shapeId, 1);
        b2Circle circle = b2Shape_GetCircle(shapeId);
        shape.setRadius(circle.radius);

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
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
