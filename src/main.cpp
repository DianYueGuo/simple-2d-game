#include <iostream>

#include <SFML/Graphics.hpp>

#include "game.hpp"


int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "My window");

    Game game(window);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        game.handleInput();
        game.update();
        game.render();
    }

    return 0;
}
