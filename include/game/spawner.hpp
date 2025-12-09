#ifndef GAME_SPAWNER_HPP
#define GAME_SPAWNER_HPP

#include <memory>
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <box2d/box2d.h>

class EatableCircle;
class EaterCircle;
class Game;

// Responsible for creating circles and spawning logic.
class Spawner {
public:
    explicit Spawner(Game& game);

    void try_add_circle_at(const sf::Vector2f& worldPos);
    void begin_add_drag_if_applicable(const sf::Vector2f& worldPos);
    void continue_add_drag(const sf::Vector2f& worldPos);
    void reset_add_drag_state();

    void sprinkle_entities(float dt);
    void ensure_minimum_eaters();
    b2Vec2 random_point_in_petri() const;
    std::unique_ptr<EaterCircle> create_eater_at(const b2Vec2& pos);
    std::unique_ptr<EatableCircle> create_eatable_at(const b2Vec2& pos, bool toxic, bool division_boost = false) const;
    void spawn_eatable_cloud(const EaterCircle& eater, std::vector<std::unique_ptr<EatableCircle>>& out);

private:
    void sprinkle_with_rate(float rate, int type, float dt);

    Game& game;
    bool add_dragging = false;
    std::optional<sf::Vector2f> last_add_world_pos;
    std::optional<sf::Vector2f> last_drag_world_pos;
    float add_drag_distance = 0.0f;
};

#endif
