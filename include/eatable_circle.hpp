#ifndef EATABLE_CIRCLE_HPP
#define EATABLE_CIRCLE_HPP

#include "drawable_circle.hpp"

class EatableCircle : public DrawableCircle {
public:
    EatableCircle(const b2WorldId &worldId, float position_x = 0.0f, float position_y = 0.0f, float radius = 1.0f, float density = 1.0f, bool toxic = false, float angle = 0.0f, bool boost_particle = false);
    void be_eaten();
    void set_eaten_by(const EaterCircle* eater) { eaten_by = eater; }
    const EaterCircle* get_eaten_by() const { return eaten_by; }
    bool is_eaten() const;
    bool is_toxic() const { return toxic; }
    void set_toxic(bool value) { toxic = value; }
    bool is_boost_particle() const { return boost_particle; }
private:
    bool eaten = false;
    bool toxic = false;
    bool boost_particle = false;
    const EaterCircle* eaten_by = nullptr;
};

#endif
