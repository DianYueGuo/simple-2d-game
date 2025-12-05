#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <optional>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "eater_circle.hpp"

#include "eatable_circle.hpp"


class Game {
public:
    enum class CursorMode {
        Add,
        Drag
    };
    enum class AddType {
        Eater,
        Eatable,
        ToxicEatable
    };

    Game();
    ~Game();
    void process_game_logic();
    void draw(sf::RenderWindow& window) const;
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);
    void set_pixles_per_meter(float ppm) { pixles_per_meter = ppm; }
    void set_time_scale(float scale) { time_scale = scale; }
    void set_brain_updates_per_sim_second(float hz) { brain_updates_per_sim_second = hz; }
    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }
    void set_cursor_mode(CursorMode mode) { cursor_mode = mode; }
    void set_add_type(AddType type) { add_type = type; }
    void set_add_eatable_area(float area) { add_eatable_area = area; }
    CursorMode get_cursor_mode() const { return cursor_mode; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
private:
    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
    float pixles_per_meter = 100.0f;
    float time_scale = 1.0f;
    float brain_updates_per_sim_second = 60.0f;
    float brain_time_accumulator = 0.0f;
    float minimum_area = 1.0f;
    CursorMode cursor_mode = CursorMode::Add;
    AddType add_type = AddType::Eater;
    bool add_dragging = false;
    std::optional<sf::Vector2f> last_add_world_pos;
    std::optional<sf::Vector2f> last_drag_world_pos;
    float add_drag_distance = 0.0f;
    float add_eatable_area = 1.0f;
    bool dragging = false;
    bool right_dragging = false;
    sf::Vector2i last_drag_pixels{};
};

#endif
