#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <optional>
#include <algorithm>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "eatable_circle.hpp"
#include <NEAT/genome.hpp>

class EaterCircle;


class Game {
public:
    enum class CursorMode {
        Add,
        Drag,
        Select
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
    void set_paused(bool p) { paused = p; }
    bool is_paused() const { return paused; }
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
    void set_add_node_probability(float p) { add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_add_node_probability() const { return add_node_probability; }
    void set_remove_node_probability(float p) { remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_remove_node_probability() const { return remove_node_probability; }
    void set_add_connection_probability(float p) { add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_add_connection_probability() const { return add_connection_probability; }
    void set_remove_connection_probability(float p) { remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_remove_connection_probability() const { return remove_connection_probability; }
    void set_tick_add_node_probability(float p) { tick_add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_node_probability() const { return tick_add_node_probability; }
    void set_tick_remove_node_probability(float p) { tick_remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_remove_node_probability() const { return tick_remove_node_probability; }
    void set_tick_add_connection_probability(float p) { tick_add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_connection_probability() const { return tick_add_connection_probability; }
    void set_tick_remove_connection_probability(float p) { tick_remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_remove_connection_probability() const { return tick_remove_connection_probability; }
    void set_init_add_node_probability(float p) { init_add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_node_probability() const { return init_add_node_probability; }
    void set_init_remove_node_probability(float p) { init_remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_remove_node_probability() const { return init_remove_node_probability; }
    void set_init_add_connection_probability(float p) { init_add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_connection_probability() const { return init_add_connection_probability; }
    void set_init_remove_connection_probability(float p) { init_remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_remove_connection_probability() const { return init_remove_connection_probability; }
    void set_init_mutation_rounds(int rounds) { init_mutation_rounds = std::clamp(rounds, 0, 100); }
    int get_init_mutation_rounds() const { return init_mutation_rounds; }
    void set_mutation_rounds(int rounds) { mutation_rounds = std::clamp(rounds, 0, 50); }
    int get_mutation_rounds() const { return mutation_rounds; }
    int get_max_generation() const { return max_generation; }
    const neat::Genome* get_max_generation_brain() const { return max_generation_brain ? &(*max_generation_brain) : nullptr; }
    std::vector<std::vector<int>>* get_neat_innovations() { return &neat_innovations; }
    int* get_neat_last_innovation_id() { return &neat_last_innov_id; }
    void set_inactivity_timeout(float t) { inactivity_timeout = std::max(0.0f, t); }
    float get_inactivity_timeout() const { return inactivity_timeout; }
    void set_linear_impulse_magnitude(float m);
    float get_linear_impulse_magnitude() const { return linear_impulse_magnitude; }
    void set_angular_impulse_magnitude(float m);
    float get_angular_impulse_magnitude() const { return angular_impulse_magnitude; }
    void set_boost_particle_impulse_fraction(float f) { boost_particle_impulse_fraction = std::clamp(f, 0.0f, 1.0f); }
    float get_boost_particle_impulse_fraction() const { return boost_particle_impulse_fraction; }
    void set_linear_damping(float d);
    float get_linear_damping() const { return linear_damping; }
    void set_angular_damping(float d);
    float get_angular_damping() const { return angular_damping; }
    void set_boost_particle_linear_damping(float d) { boost_particle_linear_damping = std::max(0.0f, d); }
    float get_boost_particle_linear_damping() const { return boost_particle_linear_damping; }
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
    void set_eater_cloud_area_percentage(float percentage) { eater_cloud_area_percentage = percentage; }
    float get_eater_cloud_area_percentage() const { return eater_cloud_area_percentage; }
    void update_max_generation_from_circle(const EatableCircle* circle);
    void recompute_max_generation();
    void set_show_true_color(bool value) { show_true_color = value; }
    bool get_show_true_color() const { return show_true_color; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
    std::size_t get_eater_count() const;
    void remove_random_percentage(float percentage);
    void remove_outside_petri();
    void set_auto_remove_outside(bool enabled) { auto_remove_outside = enabled; }
    bool get_auto_remove_outside() const { return auto_remove_outside; }
    std::size_t get_circle_count() const { return circles.size(); }
    float get_sim_time() const { return sim_time_accum; }
    float get_real_time() const { return real_time_accum; }
    float get_last_fps() const { return fps_last; }
    void accumulate_real_time(float dt);
    void frame_rendered();
    void clear_selection();
    const neat::Genome* get_selected_brain() const;
    const EaterCircle* get_selected_eater() const;
    int get_selected_generation() const;
    bool select_circle_at_world(const b2Vec2& pos);
    CursorMode get_cursor_mode() const { return cursor_mode; }
private:
    void spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out);
    b2Vec2 random_point_in_petri() const;
    void sprinkle_with_rate(float rate, AddType type, float dt);
    void sprinkle_entities(float dt);
    void update_eaters(const b2WorldId& worldId);
    void run_brain_updates(const b2WorldId& worldId, float timeStep);
    void cull_consumed();
    void remove_stopped_boost_particles();
    std::unique_ptr<EaterCircle> create_eater_at(const b2Vec2& pos);
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
    float sim_time_accum = 0.0f;
    float real_time_accum = 0.0f;
    float fps_accum_time = 0.0f;
    int fps_frames = 0;
    float fps_last = 0.0f;
    float brain_updates_per_sim_second = 10.0f;
    float brain_time_accumulator = 0.0f;
    float minimum_area = 1.0f;
    CursorMode cursor_mode = CursorMode::Add;
    AddType add_type = AddType::Eater;
    bool add_dragging = false;
    std::optional<sf::Vector2f> last_add_world_pos;
    std::optional<sf::Vector2f> last_drag_world_pos;
    float add_drag_distance = 0.0f;
    float add_eatable_area = 0.3f;
    float boost_area = 0.003f;
    float poison_death_probability = 1.0f;
    float poison_death_probability_normal = 0.01f;
    float petri_radius = 50.0f;
    float sprinkle_rate_eater = 5.0f;
    float average_eater_area = 5.0f;
    float sprinkle_rate_eatable = 10.0f;
    float sprinkle_rate_toxic = 1.5f;
    float eater_cloud_area_percentage = 70.0f;
    float add_node_probability = 0.1f;
    float remove_node_probability = 0.05f;
    float add_connection_probability = 0.1f;
    float remove_connection_probability = 0.05f;
    float tick_add_node_probability = 0.0f;
    float tick_remove_node_probability = 0.0f;
    float tick_add_connection_probability = 0.0f;
    float tick_remove_connection_probability = 0.0f;
    float init_add_node_probability = 0.1f;
    float init_remove_node_probability = 0.02f;
    float init_add_connection_probability = 0.15f;
    float init_remove_connection_probability = 0.02f;
    int init_mutation_rounds = 10;
    int mutation_rounds = 4;
    int max_generation = 0;
    std::optional<neat::Genome> max_generation_brain;
    std::vector<std::vector<int>> neat_innovations;
    int neat_last_innov_id = 0;
    bool show_true_color = false;
    float inactivity_timeout = 10.0f;
    bool auto_remove_outside = true;
    bool dragging = false;
    bool right_dragging = false;
    sf::Vector2i last_drag_pixels{};
    float circle_density = 1.0f;
    float linear_impulse_magnitude = 1.0f;
    float angular_impulse_magnitude = 1.0f;
    float linear_damping = 0.5f;
    float angular_damping = 0.5f;
    std::optional<std::size_t> selected_index;
    bool paused = false;
    float boost_particle_impulse_fraction = 0.003f;
    float boost_particle_linear_damping = 3.0f;
};

#endif
