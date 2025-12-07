#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"
#include <NEAT/genome.hpp>

#include <algorithm>
#include <array>
#include <vector>

class Game;


class EaterCircle : public EatableCircle {
public:
    EaterCircle(const b2WorldId &worldId,
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

    void process_eating(const b2WorldId &worldId, float poison_death_probability_toxic, float poison_death_probability_normal);

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
    void initialize_brain(int mutation_rounds, float add_node_p, float remove_node_p, float add_connection_p, float remove_connection_p);
    void update_brain_inputs_from_touching();
    void update_color_from_brain();

    neat::Genome brain;
    std::array<float, 29> brain_inputs{};
    std::array<float, 11> brain_outputs{};
    std::array<float, 4> memory_state{};
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
