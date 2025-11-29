#include "eater_circle.hpp"


EaterCircle::EaterCircle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    EatableCircle(worldId, position_x, position_y, radius, density, friction) {
}

void EaterCircle::process_eating() {
    for (auto* touching_circle : touching_circles) {
        if (touching_circle->getRadius() < this->getRadius()) {
            static_cast<EatableCircle*>(touching_circle)->be_eaten();
        }
    }
}
