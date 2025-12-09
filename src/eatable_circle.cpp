#include "eatable_circle.hpp"
#include "creature_circle.hpp"

EatableCircle::EatableCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, bool toxic, bool division_boost, float angle, bool boost_particle) :
    DrawableCircle(
        worldId,
        position_x,
        position_y,
        radius,
        density,
        angle,
        boost_particle ? CircleKind::BoostParticle
                       : division_boost ? CircleKind::DivisionPellet
                                        : toxic ? CircleKind::ToxicPellet
                                                : CircleKind::Pellet),
    toxic(toxic),
    division_boost(division_boost),
    boost_particle(boost_particle) {
    if (division_boost) {
        set_color_rgb(0.0f, 0.0f, 1.0f); // blue
    } else if (toxic) {
        set_color_rgb(1.0f, 0.0f, 0.0f);
    } else {
        set_color_rgb(0.0f, 1.0f, 0.0f);
    }
    smooth_display_color(1.0f);
}

void EatableCircle::be_eaten() {
    eaten = true;
    // eaten_by is set by the creature that consumed us (if applicable).
}

bool EatableCircle::is_eaten() const {
    return eaten;
}

void EatableCircle::update_kind_from_flags() {
    if (boost_particle) {
        set_kind(CircleKind::BoostParticle);
    } else if (division_boost) {
        set_kind(CircleKind::DivisionPellet);
    } else if (toxic) {
        set_kind(CircleKind::ToxicPellet);
    } else {
        set_kind(CircleKind::Pellet);
    }
}
