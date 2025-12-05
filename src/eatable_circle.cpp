#include "eatable_circle.hpp"

EatableCircle::EatableCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction, bool toxic) :
    DrawableCircle(worldId, position_x, position_y, radius, density, friction),
    toxic(toxic) {
}

void EatableCircle::be_eaten() {
    eaten = true;
}

bool EatableCircle::is_eaten() const {
    return eaten;
}
