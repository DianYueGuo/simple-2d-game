#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>


class CirclePhysics {
public:
    CirclePhysics(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f) :
        bodyId{} {
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

    ~CirclePhysics() {
        if (b2Body_IsValid(bodyId)) {
            b2DestroyBody(bodyId);
        }
    }

    CirclePhysics(const CirclePhysics&) = delete;
    CirclePhysics& operator=(const CirclePhysics&) = delete;

    CirclePhysics(CirclePhysics&& other_circle_physics) noexcept
        : bodyId(other_circle_physics.bodyId)
    {
        other_circle_physics.bodyId = (b2BodyId){};
    }

    CirclePhysics& operator=(CirclePhysics&& other_circle_physics) noexcept {
        if (this == &other_circle_physics) {
            return *this;
        }

        if (b2Body_IsValid(bodyId)) b2DestroyBody(bodyId);
        bodyId = other_circle_physics.bodyId;
        other_circle_physics.bodyId = (b2BodyId){};

        return *this;
    }

    b2Vec2 getPosition() const {
        return b2Body_GetPosition(bodyId);
    }

    float getRadius() const {
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

    std::vector<CirclePhysics> circle_physics;

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
                    circle_physics.push_back(
                        CirclePhysics(
                        worldId,
                        mouseButtonPressed->position.x,
                        mouseButtonPressed->position.y,
                        20.0f,
                        1.0f,
                        0.3f
                    ));
                }
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();

        for (const auto& circle_physics : circle_physics) {
            shape.setPosition({circle_physics.getPosition().x, circle_physics.getPosition().y});
            shape.setRadius(circle_physics.getRadius());
            window.draw(shape);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    b2DestroyWorld(worldId);

    return 0;
}
