#ifndef CELL_HPP
#define CELL_HPP

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>


class Cell {
public:
    Cell(b2WorldId worldId);
    ~Cell();

    void draw(sf::RenderWindow& window) const;
    void apply_impulse();
private:
    b2BodyId bodyId;
    b2ShapeId shapeId;
};

#endif
