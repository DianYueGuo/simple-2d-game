#include "eatable_circle.hpp"

EatableCircle::EatableCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, bool toxic, float angle, bool boost_particle) :
    DrawableCircle(worldId, position_x, position_y, radius, density, angle),
    toxic(toxic),
    boost_particle(boost_particle) {
    if (toxic) {
        set_color_rgb(1.0f, 0.0f, 0.0f);
    } else {
        set_color_rgb(0.0f, 1.0f, 0.0f);
    }
    smooth_display_color(1.0f);
}

void EatableCircle::be_eaten() {
    eaten = true;
}

bool EatableCircle::is_eaten() const {
    return eaten;
}
