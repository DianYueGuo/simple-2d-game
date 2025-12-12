#ifndef CREATURE_CIRCLE_HPP
#define CREATURE_CIRCLE_HPP

#include "eatable_circle.hpp"
#include "simulation_config.hpp"
#include <NEAT/genome.hpp>

#include <algorithm>
#include <array>
#include <vector>

class Game;


class CreatureCircle : public EatableCircle {
public:
    explicit CreatureCircle(const b2WorldId &worldId,
                float position_x = 0.0f,
                float position_y = 0.0f,
                float radius = 1.0f,
                float density = 1.0f,
                float angle = 0.0f,
                int generation = 0,
                int init_mutation_rounds = 100,
                float init_add_node_probability = 0.8f,
                float init_remove_node_probability = 0.0f,
                float init_add_connection_probability = 1.0f,
                float init_remove_connection_probability = 0.0f,
                const neat::Genome* base_brain = nullptr,
                std::vector<std::vector<int>>* innov_ids = nullptr,
                int* last_innov_id = nullptr,
                Game* owner = nullptr);

    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }
    int get_generation() const { return generation; }
    void set_generation(int g) { generation = std::max(0, g); }
    const neat::Genome& get_brain() const { return brain; }

    void process_eating(const b2WorldId &worldId, Game& game, float poison_death_probability_toxic, float poison_death_probability_normal);
    void update_inactivity(float dt, float timeout);

    void move_randomly(const b2WorldId &worldId, Game &game);
    void move_intelligently(const b2WorldId &worldId, Game &game, float dt);

    void boost_forward(const b2WorldId &worldId, Game& game);
    void divide(const b2WorldId &worldId, Game& game);

    bool is_poisoned() const { return poisoned; }
    void set_creation_time(float t) { creation_time = t; }
    float get_creation_time() const { return creation_time; }
    void set_last_division_time(float t) { last_division_time = t; }
    float get_last_division_time() const { return last_division_time; }

protected:
    bool should_draw_direction_indicator() const override { return true; }

private:
    static constexpr int SENSOR_COUNT = kColorSensorCount;
    static constexpr int MEMORY_SLOTS = 4;
    static constexpr int BRAIN_OUTPUTS = 11;
    static constexpr int SENSOR_INPUTS = SENSOR_COUNT * 3;
    static constexpr int SIZE_INPUT_INDEX = SENSOR_INPUTS;
    static constexpr int MEMORY_INPUT_START = SIZE_INPUT_INDEX + 1;
    static constexpr int BRAIN_INPUTS = SENSOR_INPUTS + 1 + MEMORY_SLOTS;

    void initialize_brain(int mutation_rounds, float add_node_p, float remove_node_p, float add_connection_p, float remove_connection_p);
    void run_brain_cycle_from_touching();
    void update_brain_inputs_from_touching();
    void apply_sensor_inputs(const std::array<std::array<float, 3>, SENSOR_COUNT>& summed_colors, const std::array<float, SENSOR_COUNT>& weights);
    void write_size_and_memory_inputs();
    void update_color_from_brain();
    bool can_eat_circle(const CirclePhysics& circle) const;
    bool has_overlap_to_eat(const CirclePhysics& circle) const;
    void consume_touching_circle(const b2WorldId &worldId, Game& game, EatableCircle& eatable, float touching_area, float poison_death_probability_toxic, float poison_death_probability_normal);
    void configure_child_after_division(CreatureCircle& child, const b2WorldId& worldId, const Game& game, float angle, const neat::Genome& parent_brain_copy) const;
    void mutate_lineage(const Game& game, CreatureCircle* child);

    neat::Genome brain;
    std::array<float, BRAIN_INPUTS> brain_inputs{};
    std::array<float, BRAIN_OUTPUTS> brain_outputs{};
    std::array<float, MEMORY_SLOTS> memory_state{};
    std::vector<std::vector<int>>* neat_innovations = nullptr;
    int* neat_last_innov_id = nullptr;
    float minimum_area = 1.0f;
    bool poisoned = false;
    int generation = 0;
    float inactivity_timer = 0.0f;
    float creation_time = 0.0f;
    float last_division_time = 0.0f;
    Game* owner_game = nullptr;
};

#endif
