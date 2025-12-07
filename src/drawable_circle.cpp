#include "drawable_circle.hpp"

#include <cstdlib>
#include <algorithm>

DrawableCircle::DrawableCircle(const b2WorldId &worldId, float position_x, float position_y, float radius, float density, float angle, CircleKind kind) :
    CirclePhysics(worldId, position_x, position_y, radius, density, angle, kind) {
    for (auto& c : color_rgb) {
        c = std::clamp(static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX), 0.0f, 1.0f);
    }
    display_color_rgb = color_rgb;
    display_color_initialized = true;
}

void DrawableCircle::draw(sf::RenderWindow& window) const {
    sf::CircleShape shape(getRadius());
    sf::Color fill{
        static_cast<std::uint8_t>((use_smoothed_display ? display_color_rgb[0] : color_rgb[0]) * 255.0f),
        static_cast<std::uint8_t>((use_smoothed_display ? display_color_rgb[1] : color_rgb[1]) * 255.0f),
        static_cast<std::uint8_t>((use_smoothed_display ? display_color_rgb[2] : color_rgb[2]) * 255.0f)
    };
    shape.setFillColor(fill);

    shape.setOrigin({getRadius(), getRadius()});
    shape.setPosition({getPosition().x, getPosition().y});
    window.draw(shape);

    if (should_draw_direction_indicator()) {
        sf::RectangleShape line({getRadius(), getRadius() / 4.0f});
        line.setFillColor(sf::Color::White);
        line.rotate(sf::radians(getAngle()));

        line.setOrigin({0, getRadius() / 4.0f / 2.0f});
        line.setPosition({getPosition().x, getPosition().y});

        window.draw(line);
    }
}

void DrawableCircle::set_color_rgb(float r, float g, float b) {
    color_rgb[0] = std::clamp(r, 0.0f, 1.0f);
    color_rgb[1] = std::clamp(g, 0.0f, 1.0f);
    color_rgb[2] = std::clamp(b, 0.0f, 1.0f);
    if (!display_color_initialized) {
        display_color_rgb = color_rgb;
        display_color_initialized = true;
    }
}

void DrawableCircle::smooth_display_color(float factor) {
    float clamped = std::clamp(factor, 0.0f, 1.0f);
    if (!display_color_initialized) {
        display_color_rgb = color_rgb;
        display_color_initialized = true;
    }
    for (int i = 0; i < 3; ++i) {
        display_color_rgb[i] += (color_rgb[i] - display_color_rgb[i]) * clamped;
    }
}
