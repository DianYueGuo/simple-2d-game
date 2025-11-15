#include "game.hpp"

#include <iostream>


Game::Game() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);

    cell = std::make_unique<Cell>(worldId);
}

Game::~Game() {
    cell.reset();
    b2DestroyWorld(worldId);
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

void Game::render(sf::RenderWindow& window) const {
    window.clear(sf::Color::Black);

    cell->draw(window);

    window.display();
}
