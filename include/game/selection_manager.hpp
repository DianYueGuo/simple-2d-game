#ifndef GAME_SELECTION_MANAGER_HPP
#define GAME_SELECTION_MANAGER_HPP

#include <memory>
#include <optional>
#include <vector>

#include <SFML/Graphics/View.hpp>
#include <box2d/box2d.h>

class EatableCircle;
class EaterCircle;
namespace neat { class Genome; }

// Manages which circle is selected and follow-target logic.
class SelectionManager {
public:
    struct Snapshot {
        const EatableCircle* circle = nullptr;
        b2Vec2 position{0.0f, 0.0f};
    };

    SelectionManager(std::vector<std::unique_ptr<EatableCircle>>& circles, float& sim_time_accum);

    void clear();
    bool select_circle_at_world(const b2Vec2& pos);
    const neat::Genome* get_selected_brain() const;
    const EaterCircle* get_selected_eater() const;
    const EaterCircle* get_oldest_largest_eater() const;
    const EaterCircle* get_oldest_smallest_eater() const;
    const EaterCircle* get_oldest_middle_eater() const;
    const EaterCircle* get_follow_target_eater() const;
    int get_selected_generation() const;
    void update_follow_view(sf::View& view) const;

    void set_follow_selected(bool v);
    bool get_follow_selected() const;
    void set_follow_oldest_largest(bool v);
    bool get_follow_oldest_largest() const;
    void set_follow_oldest_smallest(bool v);
    bool get_follow_oldest_smallest() const;
    void set_follow_oldest_middle(bool v);
    bool get_follow_oldest_middle() const;

    Snapshot capture_snapshot() const;
    void revalidate_selection(const EatableCircle* previously_selected);
    void set_selection_to_eater(const EaterCircle* eater);
    const EaterCircle* find_nearest_eater(const b2Vec2& pos) const;
    void handle_selection_after_removal(const Snapshot& snapshot, bool was_removed, const EaterCircle* preferred_fallback, const b2Vec2& fallback_position);

private:
    std::vector<std::unique_ptr<EatableCircle>>* circles;
    float* sim_time;
    std::optional<std::size_t> selected_index;
    bool follow_selected = false;
    bool follow_oldest_largest = false;
    bool follow_oldest_smallest = false;
    bool follow_oldest_middle = false;
};

#endif
