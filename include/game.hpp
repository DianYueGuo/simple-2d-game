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

class CreatureCircle;


class Game {
    friend class Spawner;
public:
    enum class CursorMode {
        Add,
        Select
    };
    enum class AddType {
        Creature,
        FoodPellet,
        ToxicPellet,
        DivisionPellet
    };
private:
    struct SimulationTiming {
        float time_scale = 1.0f;
        float sim_time_accum = 0.0f;
        float real_time_accum = 0.0f;
        float last_real_dt = 0.0f;
        float last_sim_dt = 0.0f;
        float actual_sim_speed_inst = 0.0f;
    };
    struct FpsStats {
        float accum_time = 0.0f;
        int frames = 0;
        float last = 0.0f;
    };
    struct BrainSettings {
        float updates_per_second = 10.0f;
        float time_accumulator = 0.0f;
    };
    struct CreatureSettings {
        float minimum_area = 1.0f;
        float add_eatable_area = 0.3f;
        float boost_area = 0.003f;
        float average_area = 5.0f;
    };
    struct CursorState {
        CursorMode mode = CursorMode::Add;
        AddType add_type = AddType::Creature;
    };
    struct DishSettings {
        float radius = 50.0f;
        int minimum_creature_count = 10;
        bool auto_remove_outside = true;
    };
    struct PelletSettings {
        float sprinkle_rate_eatable = 50.0f;
        float sprinkle_rate_toxic = 1.0f;
        float sprinkle_rate_division = 1.0f;
        int max_food_pellets = 200;
        int max_toxic_pellets = 50;
        int max_division_pellets = 50;
        float food_density = 0.005f;
        float toxic_density = 0.0005f;
        float division_density = 0.0005f;
        float cleanup_rate_food = 0.0f;
        float cleanup_rate_toxic = 0.0f;
        float cleanup_rate_division = 0.0f;
        std::size_t food_count_cached = 0;
        std::size_t toxic_count_cached = 0;
        std::size_t division_count_cached = 0;
    };
    struct MutationSettings {
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
    };
    struct MovementSettings {
        float circle_density = 1.0f;
        float linear_impulse_magnitude = 1.0f;
        float angular_impulse_magnitude = 1.0f;
        float linear_damping = 0.5f;
        float angular_damping = 0.5f;
        float boost_particle_impulse_fraction = 0.003f;
        float boost_particle_linear_damping = 3.0f;
    };
    struct DeathSettings {
        float poison_death_probability = 1.0f;
        float poison_death_probability_normal = 0.0f;
        float creature_cloud_area_percentage = 70.0f;
        float division_pellet_divide_probability = 1.0f;
        float inactivity_timeout = 0.1f;
    };
    struct ViewDragState {
        bool dragging = false;
        bool right_dragging = false;
        sf::Vector2i last_drag_pixels{};
    };
    struct GenerationStats {
        int max_generation = 0;
        std::optional<neat::Genome> brain;
    };
    struct InnovationState {
        std::vector<std::vector<int>> innovations;
        int last_innovation_id = 0;
    };
    struct AgeStats {
        float max_age_since_creation = 0.0f;
        float max_age_since_division = 0.0f;
    };
public:

    Game();
    ~Game();
    void process_game_logic();
    void draw(sf::RenderWindow& window) const;
    void process_input_events(sf::RenderWindow& window, const std::optional<sf::Event>& event);
    void set_time_scale(float scale) { timing.time_scale = scale; }
    float get_time_scale() const { return timing.time_scale; }
    void set_paused(bool p) { paused = p; }
    bool is_paused() const { return paused; }
    void set_brain_updates_per_sim_second(float hz) { brain.updates_per_second = hz; }
    float get_brain_updates_per_sim_second() const { return brain.updates_per_second; }
    void set_minimum_area(float area) { creature.minimum_area = area; }
    float get_minimum_area() const { return creature.minimum_area; }
    void set_cursor_mode(CursorMode mode) { cursor.mode = mode; }
    void set_add_type(AddType type) { cursor.add_type = type; }
    AddType get_add_type() const { return cursor.add_type; }
    void set_add_eatable_area(float area) { creature.add_eatable_area = area; }
    float get_add_eatable_area() const { return creature.add_eatable_area; }
    void set_poison_death_probability(float p) { death.poison_death_probability = p; }
    float get_poison_death_probability() const { return death.poison_death_probability; }
    void set_poison_death_probability_normal(float p) { death.poison_death_probability_normal = p; }
    float get_poison_death_probability_normal() const { return death.poison_death_probability_normal; }
    void set_boost_area(float area) { creature.boost_area = area; }
    void set_circle_density(float d);
    float get_circle_density() const { return movement.circle_density; }
    void set_add_node_probability(float p) { mutation.add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_add_node_probability() const { return mutation.add_node_probability; }
    void set_remove_node_probability(float p) { mutation.remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_remove_node_probability() const { return mutation.remove_node_probability; }
    void set_add_connection_probability(float p) { mutation.add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_add_connection_probability() const { return mutation.add_connection_probability; }
    void set_remove_connection_probability(float p) { mutation.remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_remove_connection_probability() const { return mutation.remove_connection_probability; }
    void set_tick_add_node_probability(float p) { mutation.tick_add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_node_probability() const { return mutation.tick_add_node_probability; }
    void set_tick_remove_node_probability(float p) { mutation.tick_remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_remove_node_probability() const { return mutation.tick_remove_node_probability; }
    void set_tick_add_connection_probability(float p) { mutation.tick_add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_add_connection_probability() const { return mutation.tick_add_connection_probability; }
    void set_tick_remove_connection_probability(float p) { mutation.tick_remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_tick_remove_connection_probability() const { return mutation.tick_remove_connection_probability; }
    void set_live_mutation_enabled(bool enabled) { mutation.live_mutation_enabled = enabled; }
    bool get_live_mutation_enabled() const { return mutation.live_mutation_enabled; }
    void set_mutate_weight_thresh(float v) { mutation.mutate_weight_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_thresh() const { return mutation.mutate_weight_thresh; }
    void set_mutate_weight_full_change_thresh(float v) { mutation.mutate_weight_full_change_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_weight_full_change_thresh() const { return mutation.mutate_weight_full_change_thresh; }
    void set_mutate_weight_factor(float v) { mutation.mutate_weight_factor = std::max(0.0f, v); }
    float get_mutate_weight_factor() const { return mutation.mutate_weight_factor; }
    void set_mutate_add_connection_iterations(int v) { mutation.mutate_add_connection_iterations = std::max(1, v); }
    int get_mutate_add_connection_iterations() const { return mutation.mutate_add_connection_iterations; }
    void set_mutate_reactivate_connection_thresh(float v) { mutation.mutate_reactivate_connection_thresh = std::clamp(v, 0.0f, 1.0f); }
    float get_mutate_reactivate_connection_thresh() const { return mutation.mutate_reactivate_connection_thresh; }
    void set_mutate_add_node_iterations(int v) { mutation.mutate_add_node_iterations = std::max(1, v); }
    int get_mutate_add_node_iterations() const { return mutation.mutate_add_node_iterations; }
    void set_mutate_allow_recurrent(bool v) { mutation.mutate_allow_recurrent = v; }
    bool get_mutate_allow_recurrent() const { return mutation.mutate_allow_recurrent; }
    void set_init_add_node_probability(float p) { mutation.init_add_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_node_probability() const { return mutation.init_add_node_probability; }
    void set_init_remove_node_probability(float p) { mutation.init_remove_node_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_remove_node_probability() const { return mutation.init_remove_node_probability; }
    void set_init_add_connection_probability(float p) { mutation.init_add_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_add_connection_probability() const { return mutation.init_add_connection_probability; }
    void set_init_remove_connection_probability(float p) { mutation.init_remove_connection_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_init_remove_connection_probability() const { return mutation.init_remove_connection_probability; }
    void set_init_mutation_rounds(int rounds) { mutation.init_mutation_rounds = std::clamp(rounds, 0, 100); }
    int get_init_mutation_rounds() const { return mutation.init_mutation_rounds; }
    void set_mutation_rounds(int rounds) { mutation.mutation_rounds = std::clamp(rounds, 0, 50); }
    int get_mutation_rounds() const { return mutation.mutation_rounds; }
    int get_max_generation() const { return generation.max_generation; }
    const neat::Genome* get_max_generation_brain() const { return generation.brain ? &(*generation.brain) : nullptr; }
    std::vector<std::vector<int>>* get_neat_innovations() { return &innovation.innovations; }
    int* get_neat_last_innovation_id() { return &innovation.last_innovation_id; }
    void set_inactivity_timeout(float t) { death.inactivity_timeout = std::max(0.0f, t); }
    float get_inactivity_timeout() const { return death.inactivity_timeout; }
    void set_linear_impulse_magnitude(float m);
    float get_linear_impulse_magnitude() const { return movement.linear_impulse_magnitude; }
    void set_angular_impulse_magnitude(float m);
    float get_angular_impulse_magnitude() const { return movement.angular_impulse_magnitude; }
    void set_boost_particle_impulse_fraction(float f) { movement.boost_particle_impulse_fraction = std::clamp(f, 0.0f, 1.0f); }
    float get_boost_particle_impulse_fraction() const { return movement.boost_particle_impulse_fraction; }
    void set_linear_damping(float d);
    float get_linear_damping() const { return movement.linear_damping; }
    void set_angular_damping(float d);
    float get_angular_damping() const { return movement.angular_damping; }
    void set_boost_particle_linear_damping(float d) { movement.boost_particle_linear_damping = std::max(0.0f, d); }
    float get_boost_particle_linear_damping() const { return movement.boost_particle_linear_damping; }
    float get_boost_area() const { return creature.boost_area; }
    void set_petri_radius(float r) { dish.radius = r; }
    float get_petri_radius() const { return dish.radius; }
    void set_minimum_creature_count(int count) { dish.minimum_creature_count = std::max(0, count); }
    int get_minimum_creature_count() const { return dish.minimum_creature_count; }
    void set_average_creature_area(float area) { creature.average_area = area; }
    float get_average_creature_area() const { return creature.average_area; }
    void set_sprinkle_rate_eatable(float r) { pellets.sprinkle_rate_eatable = r; }
    void set_sprinkle_rate_toxic(float r) { pellets.sprinkle_rate_toxic = r; }
    void set_sprinkle_rate_division(float r) { pellets.sprinkle_rate_division = r; }
    float get_sprinkle_rate_eatable() const { return pellets.sprinkle_rate_eatable; }
    float get_sprinkle_rate_toxic() const { return pellets.sprinkle_rate_toxic; }
    float get_sprinkle_rate_division() const { return pellets.sprinkle_rate_division; }
    void set_creature_cloud_area_percentage(float percentage) { death.creature_cloud_area_percentage = percentage; }
    float get_creature_cloud_area_percentage() const { return death.creature_cloud_area_percentage; }
    void set_division_pellet_divide_probability(float p) { death.division_pellet_divide_probability = std::clamp(p, 0.0f, 1.0f); }
    float get_division_pellet_divide_probability() const { return death.division_pellet_divide_probability; }
    void set_max_food_pellets(int v) { pellets.max_food_pellets = std::max(0, v); }
    int get_max_food_pellets() const { return pellets.max_food_pellets; }
    void set_max_toxic_pellets(int v) { pellets.max_toxic_pellets = std::max(0, v); }
    int get_max_toxic_pellets() const { return pellets.max_toxic_pellets; }
    void set_max_division_pellets(int v) { pellets.max_division_pellets = std::max(0, v); }
    int get_max_division_pellets() const { return pellets.max_division_pellets; }
    void set_food_pellet_density(float d) { pellets.food_density = std::max(0.0f, d); }
    float get_food_pellet_density() const { return pellets.food_density; }
    void set_toxic_pellet_density(float d) { pellets.toxic_density = std::max(0.0f, d); }
    float get_toxic_pellet_density() const { return pellets.toxic_density; }
    void set_division_pellet_density(float d) { pellets.division_density = std::max(0.0f, d); }
    float get_division_pellet_density() const { return pellets.division_density; }
    std::size_t get_food_pellet_count() const;
    std::size_t get_toxic_pellet_count() const;
    std::size_t get_division_pellet_count() const;
    void update_max_generation_from_circle(const EatableCircle* circle);
    void recompute_max_generation();
    void set_show_true_color(bool value) { show_true_color = value; }
    bool get_show_true_color() const { return show_true_color; }
    void add_circle(std::unique_ptr<EatableCircle> circle);
    std::size_t get_creature_count() const;
    void remove_random_percentage(float percentage);
    void remove_percentage_pellets(float percentage, bool toxic, bool division_pellet);
    void remove_outside_petri();
    void set_auto_remove_outside(bool enabled) { dish.auto_remove_outside = enabled; }
    bool get_auto_remove_outside() const { return dish.auto_remove_outside; }
    std::size_t get_circle_count() const { return circles.size(); }
    float get_sim_time() const { return timing.sim_time_accum; }
    float get_real_time() const { return timing.real_time_accum; }
    float get_actual_sim_speed() const { return timing.actual_sim_speed_inst; }
    float get_last_fps() const { return fps.last; }
    float get_longest_life_since_creation() const { return age.max_age_since_creation; }
    float get_longest_life_since_division() const { return age.max_age_since_division; }
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
    const CreatureCircle* get_selected_creature() const;
    const CreatureCircle* get_oldest_largest_creature() const;
    const CreatureCircle* get_oldest_smallest_creature() const;
    const CreatureCircle* get_oldest_middle_creature() const;
    const CreatureCircle* get_follow_target_creature() const;
    void set_selection_to_creature(const CreatureCircle* creature);
    const CreatureCircle* find_nearest_creature(const b2Vec2& pos) const;
    int get_selected_generation() const;
    bool select_circle_at_world(const b2Vec2& pos);
    CursorMode get_cursor_mode() const { return cursor.mode; }
private:
    struct RemovalResult {
        bool should_remove = false;
        const CreatureCircle* killer = nullptr;
    };

    sf::Vector2f pixel_to_world(sf::RenderWindow& window, const sf::Vector2i& pixel) const;
    void start_view_drag(const sf::Event::MouseButtonPressed& e, bool is_right_button);
    void pan_view(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void update_creatures(const b2WorldId& worldId, float dt);
    void run_brain_updates(const b2WorldId& worldId, float timeStep);
    void cull_consumed();
    void remove_stopped_boost_particles();
    void apply_impulse_magnitudes_to_circles();
    void apply_damping_to_circles();
    void adjust_pellet_count(const EatableCircle* circle, int delta);
    std::size_t get_cached_pellet_count(bool toxic, bool division) const;
    void handle_mouse_press(sf::RenderWindow& window, const sf::Event::MouseButtonPressed& e);
    void handle_mouse_release(const sf::Event::MouseButtonReleased& e);
    void handle_mouse_move(sf::RenderWindow& window, const sf::Event::MouseMoved& e);
    void handle_key_press(sf::RenderWindow& window, const sf::Event::KeyPressed& e);
    void update_max_ages();
    void adjust_cleanup_rates();
    std::size_t count_pellets(bool toxic, bool division_pellet) const;
    void erase_indices_descending(std::vector<std::size_t>& indices);
    void refresh_generation_and_age();
    RemovalResult evaluate_circle_removal(EatableCircle& circle, std::vector<std::unique_ptr<EatableCircle>>& spawned_cloud);
    void update_actual_sim_speed();

    b2WorldId worldId;
    std::vector<std::unique_ptr<EatableCircle>> circles;
    SimulationTiming timing;
    FpsStats fps;
    BrainSettings brain;
    CreatureSettings creature;
    CursorState cursor;
    DishSettings dish;
    PelletSettings pellets;
    MutationSettings mutation;
    MovementSettings movement;
    DeathSettings death;
    GenerationStats generation;
    InnovationState innovation;
    AgeStats age;
    ViewDragState view_drag;
    SelectionManager selection;
    Spawner spawner;
    bool show_true_color = false;
    bool paused = false;
};

#endif
