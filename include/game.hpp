#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "cell.hpp"

class Game {
public:
    Game();
    ~Game();

    void handleInput();
    void update();
    void render(sf::RenderWindow& window) const;
private:
    b2WorldId worldId;

    std::unique_ptr<Cell> cell;
};

#endif