#include "eater_circle.hpp"


EaterCircle::EaterCircle(b2WorldId &worldId, float position_x, float position_y, float radius, float density, float friction) :
    EatableCircle(worldId, position_x, position_y, radius, density, friction) {
}

float calculate_overlap_area(float r1, float r2, float distance) {
    if (distance >= r1 + r2) return 0.0f;
    if (distance <= fabs(r1 - r2)) return 3.14159f * fmin(r1, r2) * fmin(r1, r2);

    float r_sq1 = r1 * r1;
    float r_sq2 = r2 * r2;
    float d_sq = distance * distance;

    float part1 = r_sq1 * acos((d_sq + r_sq1 - r_sq2) / (2.0f * distance * r1));
    float part2 = r_sq2 * acos((d_sq + r_sq2 - r_sq1) / (2.0f * distance * r2));
    float part3 = 0.5f * sqrt((r1 + r2 - distance) * (r1 - r2 + distance) * (-r1 + r2 + distance) * (r1 + r2 + distance));

    return part1 + part2 - part3;
}

void EaterCircle::process_eating() {
    for (auto* touching_circle : touching_circles) {
        if (touching_circle->getRadius() < this->getRadius()) {
            float touching_area = 3.14159f * touching_circle->getRadius() * touching_circle->getRadius();
            float overlap_threshold = touching_area * 0.8f;

            float distance = b2Distance(this->getPosition(), touching_circle->getPosition());
            float overlap_area = calculate_overlap_area(this->getRadius(), touching_circle->getRadius(), distance);

            if (overlap_area >= overlap_threshold) {
                static_cast<EatableCircle*>(touching_circle)->be_eaten();
            }
        }
    }
}
