#ifndef EATABLE_CIRCLE_HPP
#define EATABLE_CIRCLE_HPP

#include "drawable_circle.hpp"

class EatableCircle : public DrawableCircle {
public:
    EatableCircle(b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f);
    void be_eaten();
    bool is_eaten() const;
private:
    bool eaten = false;
};

#endif
