#ifndef DRAWABLE_CIRCLE_HPP
#define DRAWABLE_CIRCLE_HPP

#include "circle_physics.hpp"

#include <SFML/Graphics.hpp>
#include <array>


class DrawableCircle : public CirclePhysics {
public:
    DrawableCircle(const b2WorldId &worldId,
                   float position_x = 0.0f,
                   float position_y = 0.0f,
                   float radius = 1.0f,
                   float density = 1.0f,
                   float angle = 0.0f,
                   CircleKind kind = CircleKind::Unknown);

    void draw(sf::RenderWindow& window) const;
    void set_color_rgb(float r, float g, float b);
    std::array<float, 3> get_color_rgb() const { return color_rgb; }
    std::array<float, 3> get_display_color_rgb() const { return display_color_rgb; }
    void smooth_display_color(float factor);
    void set_use_smoothed_display(bool enabled) { use_smoothed_display = enabled; }
    void set_display_mode(bool smoothed) { use_smoothed_display = smoothed; }
protected:
    std::array<float, 3> color_rgb{};
    std::array<float, 3> display_color_rgb{};
    bool display_color_initialized = false;
    bool use_smoothed_display = true;

    virtual bool should_draw_direction_indicator() const { return false; }
};

#endif
