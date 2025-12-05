#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"

#include "game.hpp"

class Game;


class EaterCircle : public EatableCircle {
public:
    EaterCircle(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    void process_eating(const b2WorldId &worldId);

    void move_randomly(const b2WorldId &worldId, Game &game);

    void boost_forward(const b2WorldId &worldId, Game& game);
};

#endif
