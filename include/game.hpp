#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <optional>
#include <algorithm>

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "eatable_circle.hpp"
#include "game/selection_manager.hpp"
#include "game/spawner.hpp"
#include <NEAT/genome.hpp>

class EaterCircle;


class Game {
    friend class Spawner;
public:
    enum class CursorMode {
        Add,
        Select
    };
    enum class AddType {
        Eater,
        Eatable,
        ToxicEatable,
        DivisionEatable
    };

    Game();
    ~Game();
    void process_game_logic();
    void draw(sf::RenderWindow& window) const;
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);
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
    void set_live_mutation_enabled(bool enabled) { live_mutation_enabled = enabled; }
    bool get_live_mutation_enabled() const { return live_mutation_enabled; }
    void set_mutate_weight_thresh(float v) { mutate_weight_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_thresh() const { return mutate_weight_thresh; }
    void set_mutate_weight_full_change_thresh(float v) { mutate_weight_full_change_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_full_change_thresh() const { return mutate_weight_full_change_thresh; }
    void set_mutate_weight_factor(float v) { mutate_weight_factor = std::max(0.0f, v); }
    float get_mutate_weight_factor() const { return mutate_weight_factor; }
    void set_mutate_add_connection_iterations(int v) { mutate_add_connection_iterations = std::max(1, v); }
    int get_mutate_add_connection_iterations() const { return mutate_add_connection_iterations; }
    void set_mutate_reactivate_connection_thresh(float v) { mutate_reactivate_connection_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_reactivate_connection_thresh() const { return mutate_reactivate_connection_thresh; }
    void set_mutate_add_node_iterations(int v) { mutate_add_node_iterations = std::max(1, v); }
    int get_mutate_add_node_iterations() const { return mutate_add_node_iterations; }
    void set_mutate_allow_recurrent(bool v) { mutate_allow_recurrent = v; }
    bool get_mutate_allow_recurrent() const { return mutate_allow_recurrent; }
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
    void set_minimum_eater_count(int count) { minimum_eater_count = std::max(0, count); }
    int get_minimum_eater_count() const { return minimum_eater_count; }
    void set_average_eater_area(float area) { average_eater_area = area; }
    float get_average_eater_area() const { return average_eater_area; }
    void set_sprinkle_rate_eatable(float r) { sprinkle_rate_eatable = r; }
    void set_sprinkle_rate_toxic(float r) { sprinkle_rate_toxic = r; }
    void set_sprinkle_rate_division(float r) { sprinkle_rate_division = r; }
    float get_sprinkle_rate_eatable() const { return sprinkle_rate_eatable; }
    float get_sprinkle_rate_toxic() const { return sprinkle_rate_toxic; }
    float get_sprinkle_rate_division() const { return sprinkle_rate_division; }
    void set_eater_cloud_area_percentage(float percentage) { eater_cloud_area_percentage = percentage; }
    float get_eater_cloud_area_percentage() const { return eater_cloud_area_percentage; }
    void set_division_pellet_divide_probability(float p) { division_pellet_divide_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_division_pellet_divide_probability() const { return division_pellet_divide_probability; }
    void set_max_food_pellets(int v) { max_food_pellets = std::max(0, v); }
    int get_max_food_pellets() const { return max_food_pellets; }
    void set_max_toxic_pellets(int v) { max_toxic_pellets = std::max(0, v); }
    int get_max_toxic_pellets() const { return max_toxic_pellets; }
    void set_max_division_pellets(int v) { max_division_pellets = std::max(0, v); }
    int get_max_division_pellets() const { return max_division_pellets; }
    void set_food_pellet_density(float d) { food_pellet_density = std::max(0.0f, d); }
    float get_food_pellet_density() const { return food_pellet_density; }
    void set_toxic_pellet_density(float d) { toxic_pellet_density = std::max(0.0f, d); }
    float get_toxic_pellet_density() const { return toxic_pellet_density; }
    void set_division_pellet_density(float d) { division_pellet_density = std::max(0.0f, d); }
    float get_division_pellet_density() const { return division_pellet_density; }
    std::size_t get_food_pellet_count() const;
    std::size_t get_toxic_pellet_count() const;
    std::size_t get_division_pellet_count() const;
    void update_max_generation_from_circle(const EatableCircle* circle);
    void recompute_max_generation();
    void set_show_true_color(bool value) { show_true_color = value; }
    bool get_show_true_color() const { return show_true_color; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
    std::size_t get_eater_count() const;
    void remove_random_percentage(float percentage);
    void remove_percentage_pellets(float percentage, bool toxic, bool division_boost);
    void remove_outside_petri();
    void set_auto_remove_outside(bool enabled) { auto_remove_outside = enabled; }
    bool get_auto_remove_outside() const { return auto_remove_outside; }
    std::size_t get_circle_count() const { return circles.size(); }
    float get_sim_time() const { return sim_time_accum; }
    float get_real_time() const { return real_time_accum; }
    float get_last_fps() const { return fps_last; }
    float get_longest_life_since_creation() const { return max_age_since_creation; }
    float get_longest_life_since_division() const { return max_age_since_division; }
    void set_follow_selected(bool v);
    bool get_follow_selected() const;
    void set_follow_oldest_largest(bool v);
    bool get_follow_oldest_largest() const;
    void set_follow_oldest_smallest(bool v);
    bool get_follow_oldest_smallest() const;
    void set_follow_oldest_middle(bool v);
    bool get_follow_oldest_middle() const;
    void accumulate_real_time(float dt);
    void frame_rendered();
    void update_follow_view(sf::View& view) const;
    void clear_selection();
    const neat::Genome* get_selected_brain() const;
    const EaterCircle* get_selected_eater() const;
    const EaterCircle* get_oldest_largest_eater() const;
    const EaterCircle* get_oldest_smallest_eater() const;
    const EaterCircle* get_oldest_middle_eater() const;
    const EaterCircle* get_follow_target_eater() const;
    void set_selection_to_eater(const EaterCircle* eater);
    const EaterCircle* find_nearest_eater(const b2Vec2& pos) const;
    int get_selected_generation() const;
    bool select_circle_at_world(const b2Vec2& pos);
    CursorMode get_cursor_mode() const { return cursor_mode; }
private:
    struct RemovalResult {
        bool should_remove = false;
        const EaterCircle* killer = nullptr;
    };

    sf::Vector2f pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const;
    void start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button);
    void pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void update_eaters(const b2WorldId& worldId, float dt);
    void run_brain_updates(const b2WorldId& worldId, float timeStep);
    void cull_consumed();
    void remove_stopped_boost_particles();
    void apply_impulse_magnitudes_to_circles();
    void apply_damping_to_circles();
    void handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e);
    void handle_mouse_release(const sf::Event::MouseButtonReleased& e);
    void handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e);
    void update_max_ages();
    void adjust_cleanup_rates();
    std::size_t count_pellets(bool toxic, bool division_boost) const;
    void erase_indices_descending(std::vector<std::size_t>& indices);
    void refresh_generation_and_age();
    RemovalResult evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud);

    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
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
    float add_eatable_area = 0.3f;
    float boost_area = 0.003f;
    float poison_death_probability = 1.0f;
    float poison_death_probability_normal = 0.0f;
    float petri_radius = 50.0f;
    int minimum_eater_count = 10;
    float average_eater_area = 5.0f;
    float sprinkle_rate_eatable = 50.0f;
    float sprinkle_rate_toxic = 1.0f;
    float sprinkle_rate_division = 1.0f;
    float eater_cloud_area_percentage = 70.0f;
    float division_pellet_divide_probability = 1.0f;
    float cleanup_rate_food = 0.0f;     // percent per second (computed)
    float cleanup_rate_toxic = 0.0f;    // percent per second (computed)
    float cleanup_rate_division = 0.0f; // percent per second (computed)
    int max_food_pellets = 200;
    int max_toxic_pellets = 50;
    int max_division_pellets = 50;
    float food_pellet_density = 0.005f;      // area per area (m^2 per m^2)
    float toxic_pellet_density = 0.0005f;    // area per area (m^2 per m^2)
    float division_pellet_density = 0.0005f; // area per area (m^2 per m^2)
    float add_node_probability = 0.1f;
    float remove_node_probability = 0.05f;
    float add_connection_probability = 0.1f;
    float remove_connection_probability = 0.05f;
    float tick_add_node_probability = 0.0f;
    float tick_remove_node_probability = 0.0f;
    float tick_add_connection_probability = 0.0f;
    float tick_remove_connection_probability = 0.0f;
    bool live_mutation_enabled = false;
    float mutate_weight_thresh = 0.8f;
    float mutate_weight_full_change_thresh = 0.1f;
    float mutate_weight_factor = 1.2f;
    int mutate_add_connection_iterations = 20;
    float mutate_reactivate_connection_thresh = 0.25f;
    int mutate_add_node_iterations = 20;
    bool mutate_allow_recurrent = false;
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
    float inactivity_timeout = 0.1f;
    bool auto_remove_outside = true;
    bool dragging = false;
    bool right_dragging = false;
    sf::Vector2i last_drag_pixels{};
    float circle_density = 1.0f;
    float linear_impulse_magnitude = 1.0f;
    float angular_impulse_magnitude = 1.0f;
    float linear_damping = 0.5f;
    float angular_damping = 0.5f;
    float boost_particle_impulse_fraction = 0.003f;
    float boost_particle_linear_damping = 3.0f;
    float max_age_since_creation = 0.0f;
    float max_age_since_division = 0.0f;
    SelectionManager selection;
    Spawner spawner;
    bool paused = false;
};

#endif
