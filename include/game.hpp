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
    float get_pixles_per_meter() const { return pixles_per_meter; }
    void set_time_scale(float scale) { time_scale = scale; }
    float get_time_scale() const { return time_scale; }
    void set_brain_updates_per_sim_second(float hz) { brain_updates_per_sim_second = hz; }
    float get_brain_updates_per_sim_second() const { return brain_updates_per_sim_second; }
    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }
    void set_cursor_mode(CursorMode mode) { cursor_mode = mode; }
    void set_add_type(AddType type) { add_type = type; }
    AddType get_add_type() const { return add_type; }
    void set_add_eatable_area(float area) { add_eatable_area = area; }
    float get_add_eatable_area() const { return add_eatable_area; }
    void set_poison_death_probability(float p) { poison_death_probability = p; }
    float get_poison_death_probability() const { return poison_death_probability; }
    void set_poison_death_probability_normal(float p) { poison_death_probability_normal = p; }
    float get_poison_death_probability_normal() const { return poison_death_probability_normal; }
    void set_boost_area(float area) { boost_area = area; }
    void set_circle_density(float d);
    float get_circle_density() const { return circle_density; }
    void set_linear_impulse_magnitude(float m);
    float get_linear_impulse_magnitude() const { return linear_impulse_magnitude; }
    void set_angular_impulse_magnitude(float m);
    float get_angular_impulse_magnitude() const { return angular_impulse_magnitude; }
    void set_linear_damping(float d);
    float get_linear_damping() const { return linear_damping; }
    void set_angular_damping(float d);
    float get_angular_damping() const { return angular_damping; }
    float get_boost_area() const { return boost_area; }
    void set_petri_radius(float r) { petri_radius = r; }
    float get_petri_radius() const { return petri_radius; }
    void set_sprinkle_rate_eater(float r) { sprinkle_rate_eater = r; }
    void set_average_eater_area(float area) { average_eater_area = area; }
    float get_average_eater_area() const { return average_eater_area; }
    void set_sprinkle_rate_eatable(float r) { sprinkle_rate_eatable = r; }
    void set_sprinkle_rate_toxic(float r) { sprinkle_rate_toxic = r; }
    float get_sprinkle_rate_eater() const { return sprinkle_rate_eater; }
    float get_sprinkle_rate_eatable() const { return sprinkle_rate_eatable; }
    float get_sprinkle_rate_toxic() const { return sprinkle_rate_toxic; }
    CursorMode get_cursor_mode() const { return cursor_mode; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
    void remove_random_percentage(float percentage);
    void remove_outside_petri();
    void set_auto_remove_outside(bool enabled) { auto_remove_outside = enabled; }
    bool get_auto_remove_outside() const { return auto_remove_outside; }
    std::size_t get_circle_count() const { return circles.size(); }
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
    void apply_impulse_magnitudes_to_circles();
    void apply_damping_to_circles();
    void handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e);
    void handle_mouse_release(const sf::Event::MouseButtonReleased& e);
    void handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e);

    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
    float pixles_per_meter = 15.0f;
    float time_scale = 1.0f;
    float brain_updates_per_sim_second = 10.0f;
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
    float poison_death_probability_normal = 0.1f;
    float petri_radius = 20.0f;
    float sprinkle_rate_eater = 0.0f;
    float average_eater_area = 1.8f;
    float sprinkle_rate_eatable = 0.0f;
    float sprinkle_rate_toxic = 0.0f;
    bool auto_remove_outside = true;
    bool dragging = false;
    bool right_dragging = false;
    sf::Vector2i last_drag_pixels{};
    float circle_density = 1.0f;
    float linear_impulse_magnitude = 5.0f;
    float angular_impulse_magnitude = 5.0f;
    float linear_damping = 0.3f;
    float angular_damping = 1.0f;
};

#endif
