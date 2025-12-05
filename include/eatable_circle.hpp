#ifndef EATABLE_CIRCLE_HPP
#define EATABLE_CIRCLE_HPP

#include "drawable_circle.hpp"

class EatableCircle : public DrawableCircle {
public:
    EatableCircle(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, float friction = 0.0f, bool toxic = false);
    void be_eaten();
    bool is_eaten() const;
    bool is_toxic() const { return toxic; }
    void set_toxic(bool value) { toxic = value; }
private:
    bool eaten = false;
    bool toxic = false;
};

#endif
