#include "game.hpp"

#include <iostream>


Game::Game(sf::RenderWindow& window) : window(window) {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);
}

void Game::handleInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        std::cout << "Space key pressed" << std::endl;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        std::cout << "Left key pressed" << std::endl;
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        std::cout << "Right key pressed" << std::endl;
    }
}

void Game::update() {
    static const float timeStep = 1.0f / 60.0f;
    static const int subStepCount = 4;

    b2World_Step(worldId, timeStep, subStepCount);
}

void Game::render() const {
    window.clear(sf::Color::Black);

    sf::CircleShape shape(50.f);
    shape.setFillColor(sf::Color(100, 250, 50));
    window.draw(shape);

    window.display();
}
