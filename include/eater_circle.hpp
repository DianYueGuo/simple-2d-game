#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"
#include "eater_brain.hpp"

#include "game.hpp"

class Game;


class EaterCircle : public EatableCircle {
public:
    EaterCircle(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }

    void process_eating(const b2WorldId &worldId);

    void move_randomly(const b2WorldId &worldId, Game &game);
    void move_intelligently(const b2WorldId &worldId, Game &game);

    void boost_forward(const b2WorldId &worldId, Game& game);
    void divide(const b2WorldId &worldId, Game& game);

private:
    void initialize_brain();
    void update_color_from_brain();

    EaterBrain brain;
    float minimum_area = 1.0f;
};

#endif
