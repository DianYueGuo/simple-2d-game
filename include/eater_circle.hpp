#ifndef EATER_CIRCLE_HPP
#define EATER_CIRCLE_HPP

#include "eatable_circle.hpp"
#include "eater_brain.hpp"

#include <algorithm>
#include <array>

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
                const EaterBrain* base_brain = nullptr);

    void set_minimum_area(float area) { minimum_area = area; }
    float get_minimum_area() const { return minimum_area; }
    int get_generation() const { return generation; }
    void set_generation(int g) { generation = std::max(0, g); }
    const EaterBrain& get_brain() const { return brain; }

    void process_eating(const b2WorldId &worldId, float poison_death_probability_toxic, float poison_death_probability_normal);

    void move_randomly(const b2WorldId &worldId, Game &game);
    void move_intelligently(const b2WorldId &worldId, Game &game, float dt);

    void boost_forward(const b2WorldId &worldId, Game& game);
    void divide(const b2WorldId &worldId, Game& game);

    bool is_poisoned() const { return poisoned; }

protected:
    bool should_draw_direction_indicator() const override { return true; }

private:
    void initialize_brain(int mutation_rounds, float add_node_p, float remove_node_p, float add_connection_p, float remove_connection_p);
    void update_brain_inputs_from_touching();
    void update_color_from_brain();

    EaterBrain brain;
    float minimum_area = 1.0f;
    bool poisoned = false;
    int generation = 0;
    float inactivity_timer = 0.0f;
};

#endif
