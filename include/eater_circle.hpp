#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"


class EaterCircle : public EatableCircle {
public:
    EaterCircle(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);

    void process_eating();
};

#endif
