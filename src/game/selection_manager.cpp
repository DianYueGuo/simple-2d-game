#include "game/selection_manager.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "creature_circle.hpp"
#include "eatable_circle.hpp"

SelectionManager::SelectionManager(std::vector<std::unique_ptr<EatableCircle>>& circles, float& sim_time_accum)
    : circles(&circles), sim_time(&sim_time_accum) {}

void SelectionManager::clear() {
    selected_index.reset();
}

bool SelectionManager::select_circle_at_world(const b2Vec2& pos) {
    if (!circles) return false;
    std::optional<std::size_t> hit;
    float best_dist2 = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < circles->size(); ++i) {
        const auto& c = (*circles)[i];
        b2Vec2 p = c->getPosition();
        float dx = p.x - pos.x;
        float dy = p.y - pos.y;
        float dist2 = dx * dx + dy * dy;
        float r = c->getRadius();
        if (dist2 <= r * r && dist2 < best_dist2) {
            hit = i;
            best_dist2 = dist2;
        }
    }
    selected_index = hit;
    return selected_index.has_value();
}

const neat::Genome* SelectionManager::get_selected_brain() const {
    if (!circles || !selected_index || *selected_index >= circles->size()) {
        return nullptr;
    }
    const auto* base = (*circles)[*selected_index].get();
    if (base && base->get_kind() == CircleKind::Creature) {
        const auto* creature = static_cast<const CreatureCircle*>(base);
        return &creature->get_brain();
    }
    return nullptr;
}

const CreatureCircle* SelectionManager::get_selected_creature() const {
    if (!circles || !selected_index || *selected_index >= circles->size()) {
        return nullptr;
    }
    const auto* base = (*circles)[*selected_index].get();
    if (base && base->get_kind() == CircleKind::Creature) {
        return static_cast<const CreatureCircle*>(base);
    }
    return nullptr;
}

const CreatureCircle* SelectionManager::get_oldest_largest_creature() const {
    if (!circles || !sim_time) return nullptr;
    const CreatureCircle* best = nullptr;
    float best_age = -1.0f;
    float best_area = -1.0f;
    for (const auto& c : *circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(c.get());
            float age = std::max(0.0f, *sim_time - creature->get_creation_time());
            float area = creature->getArea();
            if (age > best_age || (std::abs(age - best_age) < 1e-6f && area > best_area)) {
                best = creature;
                best_age = age;
                best_area = area;
            }
        }
    }
    return best;
}

const CreatureCircle* SelectionManager::get_oldest_smallest_creature() const {
    if (!circles || !sim_time) return nullptr;
    const CreatureCircle* best = nullptr;
    float best_age = -1.0f;
    float best_area = std::numeric_limits<float>::max();
    constexpr float eps = 1e-6f;
    for (const auto& c : *circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(c.get());
            float age = std::max(0.0f, *sim_time - creature->get_creation_time());
            float area = creature->getArea();
            if (age > best_age + eps || (std::abs(age - best_age) <= eps && area < best_area)) {
                best_age = age;
                best_area = area;
                best = creature;
            }
        }
    }
    return best;
}

const CreatureCircle* SelectionManager::get_oldest_middle_creature() const {
    if (!circles || !sim_time) return nullptr;
    std::vector<std::pair<float, const CreatureCircle*>> candidates;
    float best_age = -1.0f;
    constexpr float eps = 1e-6f;
    for (const auto& c : *circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(c.get());
            float age = std::max(0.0f, *sim_time - creature->get_creation_time());
            float area = creature->getArea();
            if (age > best_age + eps) {
                best_age = age;
                candidates.clear();
                candidates.emplace_back(area, creature);
            } else if (std::abs(age - best_age) <= eps) {
                candidates.emplace_back(area, creature);
            }
        }
    }
    if (candidates.empty()) return nullptr;
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    return candidates[candidates.size() / 2].second;
}

const CreatureCircle* SelectionManager::get_follow_target_creature() const {
    if (follow_selected) {
        return get_selected_creature();
    }
    return nullptr;
}

int SelectionManager::get_selected_generation() const {
    if (!circles || !selected_index || *selected_index >= circles->size()) {
        return -1;
    }
    const auto* base = (*circles)[*selected_index].get();
    if (base && base->get_kind() == CircleKind::Creature) {
        return static_cast<const CreatureCircle*>(base)->get_generation();
    }
    return -1;
}

void SelectionManager::update_follow_view(sf::View& view) const {
    if (const CreatureCircle* creature = get_follow_target_creature()) {
        b2Vec2 p = creature->getPosition();
        view.setCenter({p.x, p.y});
    }
}

void SelectionManager::set_follow_selected(bool v) {
    follow_selected = v;
}

bool SelectionManager::get_follow_selected() const {
    return follow_selected;
}

SelectionManager::Snapshot SelectionManager::capture_snapshot() const {
    Snapshot snapshot{};
    if (selected_index && circles && *selected_index < circles->size()) {
        snapshot.circle = (*circles)[*selected_index].get();
        snapshot.position = snapshot.circle->getPosition();
    }
    return snapshot;
}

void SelectionManager::revalidate_selection(const EatableCircle* previously_selected) {
    if (!circles || !previously_selected) {
        return;
    }
    selected_index.reset();
    for (std::size_t i = 0; i < circles->size(); ++i) {
        if ((*circles)[i].get() == previously_selected) {
            selected_index = i;
            break;
        }
    }
}

void SelectionManager::set_selection_to_creature(const CreatureCircle* creature) {
    if (!circles) return;
    if (!creature) {
        selected_index.reset();
        return;
    }
    for (std::size_t i = 0; i < circles->size(); ++i) {
        if ((*circles)[i].get() == creature) {
            selected_index = i;
            return;
        }
    }
    selected_index.reset();
}

const CreatureCircle* SelectionManager::find_nearest_creature(const b2Vec2& pos) const {
    if (!circles) return nullptr;
    const CreatureCircle* best = nullptr;
    float best_dist2 = std::numeric_limits<float>::max();
    for (const auto& c : *circles) {
        if (c && c->get_kind() == CircleKind::Creature) {
            auto* creature = static_cast<const CreatureCircle*>(c.get());
            b2Vec2 p = creature->getPosition();
            float dx = p.x - pos.x;
            float dy = p.y - pos.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < best_dist2) {
                best_dist2 = d2;
                best = creature;
            }
        }
    }
    return best;
}

void SelectionManager::handle_selection_after_removal(const Snapshot& snapshot, bool was_removed, const CreatureCircle* preferred_fallback, const b2Vec2& fallback_position) {
    if (was_removed) {
        if (follow_selected) {
            const CreatureCircle* fallback = preferred_fallback;
            if (!fallback) {
                fallback = find_nearest_creature(fallback_position);
            }
            set_selection_to_creature(fallback);
        } else {
            selected_index.reset();
        }
    } else if (snapshot.circle) {
        revalidate_selection(snapshot.circle);
    }
}
