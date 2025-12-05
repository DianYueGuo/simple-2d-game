#ifndef GAME_HPP
#define GAME_HPP

#include <vector>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "eater_circle.hpp"

#include "eatable_circle.hpp"


class Game {
public:
    Game();
    ~Game();
    void process_game_logic();
    void draw(sf::RenderWindow& window) const;
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);
    void set_pixles_per_meter(float ppm) { pixles_per_meter = ppm; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
private:
    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
    float pixles_per_meter = 100.0f;
};

#endif
