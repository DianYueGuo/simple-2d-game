#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <optional>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "eatable_circle.hpp"

class EaterCircle;


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
    void set_poison_death_probability(float p) { poison_death_probability = p; }
    void set_poison_death_probability_normal(float p) { poison_death_probability_normal = p; }
    void set_boost_area(float area) { boost_area = area; }
    float get_boost_area() const { return boost_area; }
    void set_petri_radius(float r) { petri_radius = r; }
    float get_petri_radius() const { return petri_radius; }
    void set_sprinkle_rate_eater(float r) { sprinkle_rate_eater = r; }
    void set_sprinkle_rate_eatable(float r) { sprinkle_rate_eatable = r; }
    void set_sprinkle_rate_toxic(float r) { sprinkle_rate_toxic = r; }
    CursorMode get_cursor_mode() const { return cursor_mode; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
private:
    void spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out);
    b2Vec2 random_point_in_petri() const;
    void sprinkle_with_rate(float rate, AddType type, float dt);
    void sprinkle_entities(float dt);
    void update_eaters(const b2WorldId& worldId);
    void run_brain_updates(const b2WorldId& worldId, float timeStep);
    void cull_consumed();
    std::unique_ptr<EaterCircle> create_eater_at(const b2Vec2& pos) const;
    std::unique_ptr<EatableCircle> create_eatable_at(const b2Vec2& pos, bool toxic) const;
    void handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e);
    void handle_mouse_release(const sf::Event::MouseButtonReleased& e);
    void handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e);

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
    float boost_area = 0.3f;
    float poison_death_probability = 1.0f;
    float poison_death_probability_normal = 0.0f;
    float petri_radius = 20.0f;
    float sprinkle_rate_eater = 0.0f;
    float sprinkle_rate_eatable = 0.0f;
    float sprinkle_rate_toxic = 0.0f;
    bool dragging = false;
    bool right_dragging = false;
    sf::Vector2i last_drag_pixels{};
};

#endif
