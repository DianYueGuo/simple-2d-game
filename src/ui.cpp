#include <imgui.h>
#include <imgui-SFML.h>

#include "ui.hpp"
#include "creature_circle.hpp"

namespace {
struct UiState {
    int cursor_mode = static_cast<int>(Game::CursorMode::Add);
    int add_type = static_cast<int>(Game::AddType::Creature);
    float eatable_area = 1.0f;
    float delete_percentage = 10.0f;
    float petri_radius = 0.0f;
    float time_scale_display = 0.0f;
    float time_scale_requested = 0.0f;
    float brain_updates_per_sim_second = 0.0f;
    float minimum_area = 0.0f;
    float average_creature_area = 0.0f;
    float boost_area = 0.0f;
    float poison_death_probability = 0.0f;
    float poison_death_probability_normal = 0.0f;
    float creature_cloud_area_percentage = 0.0f;
    float division_pellet_divide_probability = 1.0f;
    float add_node_probability = 0.0f;
    float remove_node_probability = 0.0f;
    float add_connection_probability = 0.0f;
    float remove_connection_probability = 0.0f;
    float tick_add_node_probability = 0.0f;
    float tick_remove_node_probability = 0.0f;
    float tick_add_connection_probability = 0.0f;
    float tick_remove_connection_probability = 0.0f;
    float init_add_node_probability = 0.0f;
    float init_remove_node_probability = 0.0f;
    float init_add_connection_probability = 0.0f;
    float init_remove_connection_probability = 0.0f;
    int init_mutation_rounds = 0;
    int mutation_rounds = 0;
    float mutate_weight_thresh = 0.0f;
    float mutate_weight_full_change_thresh = 0.0f;
    float mutate_weight_factor = 0.0f;
    int mutate_add_connection_iterations = 0;
    float mutate_reactivate_connection_thresh = 0.0f;
    int mutate_add_node_iterations = 0;
    bool mutate_allow_recurrent = false;
    bool show_true_color = false;
    float inactivity_timeout = 0.0f;
    float boost_particle_impulse_fraction = 0.2f;
    float boost_particle_linear_damping = 0.5f;
    float circle_density = 0.0f;
    float linear_impulse = 0.0f;
    float angular_impulse = 0.0f;
    float linear_damping = 0.0f;
    float angular_damping = 0.0f;
    int minimum_creatures = 0;
    float sprinkle_rate_eatable = 50.0f;
    float sprinkle_rate_toxic = 0.0f;
    float sprinkle_rate_division = 0.0f;
    float cleanup_pct_food = 10.0f;
    float cleanup_pct_toxic = 10.0f;
    float cleanup_pct_division = 10.0f;
    float cleanup_interval = 30.0f; // deprecated, kept for state init
    int max_food_pellets = 0;
    int max_toxic_pellets = 0;
    int max_division_pellets = 0;
    float food_density = 0.0f;
    float toxic_density = 0.0f;
    float division_density = 0.0f;
    bool initialized = false;
    int follow_mode = 0; // 0:none, 1:selected, 2:oldest largest, 3:oldest smallest, 4:oldest median
    bool live_mutation_enabled = false;
};

enum class Preset {
    Default,
    Peaceful,
    ToxicHeavy,
    DivisionTest
};

void show_hover_text(const char* description) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", description);
    }
}

template <typename T, typename Setter>
void set_and_apply(T& field, T value, Setter setter) {
    field = value;
    setter(value);
}

void set_time_scale(UiState& state, Game& game, float value) {
    state.time_scale_display = value;
    state.time_scale_requested = value;
    game.set_time_scale(value);
}

void apply_preset(Preset preset, UiState& state, Game& game) {
    switch (preset) {
        case Preset::Default:
            set_time_scale(state, game, 1.0f);
            set_and_apply(state.brain_updates_per_sim_second, 10.0f, [&](float v) { game.set_brain_updates_per_sim_second(v); });
            set_and_apply(state.average_creature_area, 5.0f, [&](float v) { game.set_average_creature_area(v); });
            set_and_apply(state.boost_area, 0.05f, [&](float v) { game.set_boost_area(v); });
            set_and_apply(state.boost_particle_impulse_fraction, 0.02f, [&](float v) { game.set_boost_particle_impulse_fraction(v); });
            set_and_apply(state.boost_particle_linear_damping, 2.0f, [&](float v) { game.set_boost_particle_linear_damping(v); });
            set_and_apply(state.poison_death_probability, 0.3f, [&](float v) { game.set_poison_death_probability(v); });
            set_and_apply(state.poison_death_probability_normal, 0.2f, [&](float v) { game.set_poison_death_probability_normal(v); });
            set_and_apply(state.minimum_creatures, 12, [&](int v) { game.set_minimum_creature_count(v); });
            set_and_apply(state.food_density, 0.02f, [&](float v) { game.set_food_pellet_density(v); });
            set_and_apply(state.toxic_density, 0.005f, [&](float v) { game.set_toxic_pellet_density(v); });
            set_and_apply(state.division_density, 0.003f, [&](float v) { game.set_division_pellet_density(v); });
            set_and_apply(state.division_pellet_divide_probability, 0.4f, [&](float v) { game.set_division_pellet_divide_probability(v); });
            break;
        case Preset::Peaceful:
            set_time_scale(state, game, 1.0f);
            set_and_apply(state.brain_updates_per_sim_second, 6.0f, [&](float v) { game.set_brain_updates_per_sim_second(v); });
            set_and_apply(state.boost_area, 0.02f, [&](float v) { game.set_boost_area(v); });
            set_and_apply(state.boost_particle_impulse_fraction, 0.01f, [&](float v) { game.set_boost_particle_impulse_fraction(v); });
            set_and_apply(state.poison_death_probability, 0.05f, [&](float v) { game.set_poison_death_probability(v); });
            set_and_apply(state.poison_death_probability_normal, 0.02f, [&](float v) { game.set_poison_death_probability_normal(v); });
            set_and_apply(state.minimum_creatures, 8, [&](int v) { game.set_minimum_creature_count(v); });
            set_and_apply(state.food_density, 0.03f, [&](float v) { game.set_food_pellet_density(v); });
            set_and_apply(state.toxic_density, 0.0f, [&](float v) { game.set_toxic_pellet_density(v); });
            set_and_apply(state.division_density, 0.001f, [&](float v) { game.set_division_pellet_density(v); });
            set_and_apply(state.division_pellet_divide_probability, 0.25f, [&](float v) { game.set_division_pellet_divide_probability(v); });
            break;
        case Preset::ToxicHeavy:
            set_time_scale(state, game, 1.0f);
            set_and_apply(state.brain_updates_per_sim_second, 12.0f, [&](float v) { game.set_brain_updates_per_sim_second(v); });
            set_and_apply(state.boost_area, 0.08f, [&](float v) { game.set_boost_area(v); });
            set_and_apply(state.poison_death_probability, 0.7f, [&](float v) { game.set_poison_death_probability(v); });
            set_and_apply(state.poison_death_probability_normal, 0.5f, [&](float v) { game.set_poison_death_probability_normal(v); });
            set_and_apply(state.creature_cloud_area_percentage, 60.0f, [&](float v) { game.set_creature_cloud_area_percentage(v); });
            set_and_apply(state.minimum_creatures, 18, [&](int v) { game.set_minimum_creature_count(v); });
            set_and_apply(state.food_density, 0.01f, [&](float v) { game.set_food_pellet_density(v); });
            set_and_apply(state.toxic_density, 0.015f, [&](float v) { game.set_toxic_pellet_density(v); });
            set_and_apply(state.division_density, 0.0f, [&](float v) { game.set_division_pellet_density(v); });
            set_and_apply(state.division_pellet_divide_probability, 0.2f, [&](float v) { game.set_division_pellet_divide_probability(v); });
            break;
        case Preset::DivisionTest:
            set_time_scale(state, game, 1.0f);
            set_and_apply(state.brain_updates_per_sim_second, 8.0f, [&](float v) { game.set_brain_updates_per_sim_second(v); });
            set_and_apply(state.boost_area, 0.04f, [&](float v) { game.set_boost_area(v); });
            set_and_apply(state.minimum_creatures, 15, [&](int v) { game.set_minimum_creature_count(v); });
            set_and_apply(state.food_density, 0.01f, [&](float v) { game.set_food_pellet_density(v); });
            set_and_apply(state.toxic_density, 0.002f, [&](float v) { game.set_toxic_pellet_density(v); });
            set_and_apply(state.division_density, 0.02f, [&](float v) { game.set_division_pellet_density(v); });
            set_and_apply(state.division_pellet_divide_probability, 0.9f, [&](float v) { game.set_division_pellet_divide_probability(v); });
            set_and_apply(state.mutation_rounds, 5, [&](int v) { game.set_mutation_rounds(v); });
            break;
    }
}

void render_brain_graph(const neat::Genome& brain) {
    if (ImGui::BeginChild("BrainGraphGeneric", ImVec2(0, 220), true)) {
        // Layout nodes by layer: inputs (layer 0), hidden (1..max), outputs (max+1).
        int maxLayer = 1;
        for (const auto& n : brain.nodes) {
            if (n.layer > maxLayer) maxLayer = n.layer;
        }
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 origin = ImGui::GetCursorScreenPos();
        float pad = 10.0f;
        ImVec2 min = {origin.x + pad, origin.y + pad};
        ImVec2 max = {origin.x + std::max(avail.x - pad, 10.0f), origin.y + std::max(avail.y - pad, 10.0f)};

        struct DrawNode { ImVec2 pos; int id; int layer; };
        std::vector<DrawNode> drawNodes;
        drawNodes.reserve(brain.nodes.size());

        // Bucket nodes by layer
        std::vector<std::vector<int>> layerBuckets(maxLayer + 1);
        for (size_t i = 0; i < brain.nodes.size(); ++i) {
            int layer = std::max(0, brain.nodes[i].layer);
            if (layer >= (int)layerBuckets.size()) layerBuckets.resize(layer + 1);
            layerBuckets[layer].push_back(static_cast<int>(i));
        }

        // Position nodes
        for (int layer = 0; layer < (int)layerBuckets.size(); ++layer) {
            float x = (layerBuckets.size() > 1)
                          ? min.x + (max.x - min.x) * (float(layer) / float(layerBuckets.size() - 1))
                          : (min.x + max.x) * 0.5f;
            int count = (int)layerBuckets[layer].size();
            for (int idx = 0; idx < count; ++idx) {
                float y = (count > 1)
                              ? min.y + (max.y - min.y) * (float(idx) / float(count - 1))
                              : (min.y + max.y) * 0.5f;
                drawNodes.push_back({ImVec2{x, y}, layerBuckets[layer][idx], layer});
            }
        }

        auto findPos = [&](int nodeId) -> ImVec2 {
            for (auto& dn : drawNodes) {
                if (dn.id == nodeId) return dn.pos;
            }
            return ImVec2{min.x, min.y};
        };

        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Draw connections first
        for (const auto& c : brain.connections) {
            if (!c.enabled) continue;
            ImVec2 p1 = findPos(c.inNodeId);
            ImVec2 p2 = findPos(c.outNodeId);
            float w = std::clamp(std::fabs(c.weight), 0.0f, 5.0f);
            float alpha = std::clamp(std::fabs(c.weight), 0.1f, 1.0f);
            ImU32 col = ImGui::GetColorU32(ImVec4(c.weight >= 0 ? 0.2f : 0.8f, c.weight >= 0 ? 0.8f : 0.2f, 0.2f, alpha));
            dl->AddLine(p1, p2, col, 1.0f + w * 0.3f);
        }

        // Draw nodes
        for (auto& dn : drawNodes) {
            ImU32 col = ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
            dl->AddCircleFilled(dn.pos, 6.0f, col);
        }
    }
    ImGui::EndChild();
}

void render_cursor_controls(Game& game, UiState& state) {
    bool cursor_mode_changed = false;
    if (ImGui::RadioButton("Manual spawning", state.cursor_mode == static_cast<int>(Game::CursorMode::Add))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Add);
        cursor_mode_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Select", state.cursor_mode == static_cast<int>(Game::CursorMode::Select))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Select);
        cursor_mode_changed = true;
    }
    show_hover_text("Add mode places new circles; Select lets you pick existing circles.");
    if (cursor_mode_changed) {
        game.set_cursor_mode(static_cast<Game::CursorMode>(state.cursor_mode));
    }

    if (state.cursor_mode == static_cast<int>(Game::CursorMode::Add)) {
        bool add_type_changed = false;
        if (ImGui::RadioButton("Creature", state.add_type == static_cast<int>(Game::AddType::Creature))) {
            state.add_type = static_cast<int>(Game::AddType::Creature);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Food pellet", state.add_type == static_cast<int>(Game::AddType::FoodPellet))) {
            state.add_type = static_cast<int>(Game::AddType::FoodPellet);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Toxic pellet", state.add_type == static_cast<int>(Game::AddType::ToxicPellet))) {
            state.add_type = static_cast<int>(Game::AddType::ToxicPellet);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Division pellet", state.add_type == static_cast<int>(Game::AddType::DivisionPellet))) {
            state.add_type = static_cast<int>(Game::AddType::DivisionPellet);
            add_type_changed = true;
        }
        show_hover_text("Choose what to place when clicking in Add mode.");
        if (add_type_changed) {
            game.set_add_type(static_cast<Game::AddType>(state.add_type));
        }
    }
}

void initialize_state(UiState& state, Game& game) {
    if (state.initialized) return;

    state.cursor_mode = static_cast<int>(game.get_cursor_mode());
    state.add_type = static_cast<int>(game.get_add_type());
    state.eatable_area = game.get_add_eatable_area();
    state.petri_radius = game.get_petri_radius();
    state.time_scale_requested = game.get_time_scale();
    state.time_scale_display = state.time_scale_requested;
    state.brain_updates_per_sim_second = game.get_brain_updates_per_sim_second();
    state.minimum_area = game.get_minimum_area();
    state.average_creature_area = game.get_average_creature_area();
    state.boost_area = game.get_boost_area();
    state.poison_death_probability = game.get_poison_death_probability();
    state.poison_death_probability_normal = game.get_poison_death_probability_normal();
    state.creature_cloud_area_percentage = game.get_creature_cloud_area_percentage();
    state.division_pellet_divide_probability = game.get_division_pellet_divide_probability();
    state.add_node_probability = game.get_add_node_probability();
    state.remove_node_probability = game.get_remove_node_probability();
    state.add_connection_probability = game.get_add_connection_probability();
    state.remove_connection_probability = game.get_remove_connection_probability();
    state.tick_add_node_probability = game.get_tick_add_node_probability();
    state.tick_remove_node_probability = game.get_tick_remove_node_probability();
    state.tick_add_connection_probability = game.get_tick_add_connection_probability();
    state.tick_remove_connection_probability = game.get_tick_remove_connection_probability();
    state.live_mutation_enabled = game.get_live_mutation_enabled();
    state.init_add_node_probability = game.get_init_add_node_probability();
    state.init_remove_node_probability = game.get_init_remove_node_probability();
    state.init_add_connection_probability = game.get_init_add_connection_probability();
    state.init_remove_connection_probability = game.get_init_remove_connection_probability();
    state.init_mutation_rounds = game.get_init_mutation_rounds();
    state.mutation_rounds = game.get_mutation_rounds();
    state.mutate_weight_thresh = game.get_mutate_weight_thresh();
    state.mutate_weight_full_change_thresh = game.get_mutate_weight_full_change_thresh();
    state.mutate_weight_factor = game.get_mutate_weight_factor();
    state.mutate_add_connection_iterations = game.get_mutate_add_connection_iterations();
    state.mutate_reactivate_connection_thresh = game.get_mutate_reactivate_connection_thresh();
    state.mutate_add_node_iterations = game.get_mutate_add_node_iterations();
    state.mutate_allow_recurrent = game.get_mutate_allow_recurrent();
    state.show_true_color = game.get_show_true_color();
    state.inactivity_timeout = game.get_inactivity_timeout();
    state.boost_particle_impulse_fraction = game.get_boost_particle_impulse_fraction();
    state.boost_particle_linear_damping = game.get_boost_particle_linear_damping();
    state.circle_density = game.get_circle_density();
    state.linear_impulse = game.get_linear_impulse_magnitude();
    state.angular_impulse = game.get_angular_impulse_magnitude();
    state.linear_damping = game.get_linear_damping();
    state.angular_damping = game.get_angular_damping();
    state.minimum_creatures = game.get_minimum_creature_count();
    state.sprinkle_rate_eatable = game.get_sprinkle_rate_eatable();
    state.sprinkle_rate_toxic = game.get_sprinkle_rate_toxic();
    state.sprinkle_rate_division = game.get_sprinkle_rate_division();
    state.cleanup_pct_food = 0.0f;
    state.cleanup_pct_toxic = 0.0f;
    state.cleanup_pct_division = 0.0f;
    state.cleanup_interval = 0.0f;
    state.max_food_pellets = game.get_max_food_pellets();
    state.max_toxic_pellets = game.get_max_toxic_pellets();
    state.max_division_pellets = game.get_max_division_pellets();
    state.food_density = game.get_food_pellet_density();
    state.toxic_density = game.get_toxic_pellet_density();
    state.division_density = game.get_division_pellet_density();
    if (game.get_follow_selected()) {
        state.follow_mode = 1;
    } else if (game.get_follow_oldest_largest()) {
        state.follow_mode = 2;
    } else if (game.get_follow_oldest_smallest()) {
        state.follow_mode = 3;
    } else if (game.get_follow_oldest_middle()) {
        state.follow_mode = 4;
    } else {
        state.follow_mode = 0;
    }
    state.initialized = true;
}

void render_view_controls(sf::RenderWindow& window, sf::View& view, Game& game, UiState& state) {
    if (ImGui::Button("Reset view to center")) {
        view = window.getView();
        float aspect = static_cast<float>(window.getSize().x) / static_cast<float>(window.getSize().y);
        float world_height = game.get_petri_radius() * 2.0f;
        float world_width = world_height * aspect;
        view.setSize({world_width, world_height});
        view.setCenter({0.0f, 0.0f});
        window.setView(view);
    }
    show_hover_text("Recenter and reset the camera zoom to fit the dish.");
    if (ImGui::Checkbox("Show true color (disable smoothing)", &state.show_true_color)) {
        game.set_show_true_color(state.show_true_color);
    }
    show_hover_text("Toggle between smoothed display color and raw brain output color.");
}

void render_simulation_controls(Game& game, UiState& state) {
    bool paused = game.is_paused();
    if (ImGui::Checkbox("Pause simulation", &paused)) {
        game.set_paused(paused);
    }
    show_hover_text("Stop simulation updates so you can inspect selected creature info.");
    if (ImGui::SliderFloat("Simulation speed", &state.time_scale_display, 0.01f, 1000.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        state.time_scale_requested = state.time_scale_display;
        game.set_time_scale(state.time_scale_requested);
    }
    bool sim_speed_active = ImGui::IsItemActive();
    show_hover_text("Multiplies the physics time step; lower values slow everything down.");
    const float actual_sim_speed = game.get_actual_sim_speed();
    if (!paused && !sim_speed_active && actual_sim_speed > 0.0f) {
        constexpr float slowdown_threshold = 1.0f; // If we fall 10% short, keep the slider honest.
        const float requested_speed = state.time_scale_requested;
        if (actual_sim_speed < requested_speed * slowdown_threshold) {
            state.time_scale_display = std::clamp(actual_sim_speed, 0.01f, 1000.0f);
        } else {
            state.time_scale_display = requested_speed;
        }
    }
}

void render_spawning_region(Game& game, UiState& state) {
    if (ImGui::SliderFloat("Region radius (m)", &state.petri_radius, 1.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        game.set_petri_radius(state.petri_radius);
    }
    show_hover_text("Size of the petri dish in world meters.");

    bool auto_remove_outside = game.get_auto_remove_outside();
    if (ImGui::Checkbox("Auto-remove outside radius", &auto_remove_outside)) {
        game.set_auto_remove_outside(auto_remove_outside);
    }
    show_hover_text("Automatically culls any circle that leaves the dish boundary.");
}

void render_preset_buttons(Game& game, UiState& state) {
    if (ImGui::Button("Default mix")) {
        apply_preset(Preset::Default, state, game);
    }
    ImGui::SameLine();
    if (ImGui::Button("Peaceful / growth")) {
        apply_preset(Preset::Peaceful, state, game);
    }
    ImGui::SameLine();
    if (ImGui::Button("Toxic challenge")) {
        apply_preset(Preset::ToxicHeavy, state, game);
    }
    ImGui::SameLine();
    if (ImGui::Button("Division stress test")) {
        apply_preset(Preset::DivisionTest, state, game);
    }
}

void render_overview_tab(Game& game, UiState& state) {
    if (!ImGui::BeginTabItem("Overview")) {
        return;
    }

    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Object count: %zu", game.get_circle_count());
        show_hover_text("How many circles currently exist inside the dish.");
        ImGui::Text("Creatures: %zu", game.get_creature_count());
        show_hover_text("Number of creature circles currently alive.");
        ImGui::Text("Current pellets - food: %zu  toxic: %zu  division: %zu",
                    game.get_food_pellet_count(),
                    game.get_toxic_pellet_count(),
                    game.get_division_pellet_count());
        show_hover_text("Live counts for pellet types currently in the dish.");
        ImGui::Text("Sim time: %.2fs  Real time: %.2fs  FPS: %.1f", game.get_sim_time(), game.get_real_time(), game.get_last_fps());
        show_hover_text("Sim time is the accumulated simulated seconds; real is wall time since start.");
        ImGui::Text("Actual sim speed: %.2fx", game.get_actual_sim_speed());
        show_hover_text("Instantaneous simulated seconds per real second using the last frame's dt.");
        ImGui::Text("Longest life  creation/division: %.2fs / %.2fs",
                    game.get_longest_life_since_creation(),
                    game.get_longest_life_since_division());
        show_hover_text("Longest survival among creatures since spawn and since their last division.");
        ImGui::Text("Max generation: %d", game.get_max_generation());
        show_hover_text("Highest division count reached by any creature so far.");
    }

    if (ImGui::CollapsingHeader("Follow targets & selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        int follow_mode = state.follow_mode;
        const char* follow_labels[] = {
            "None",
            "Selected creature",
            "Oldest (largest if tie)",
            "Oldest (smallest if tie)",
            "Oldest (median size)"
        };
        if (ImGui::BeginTable("FollowModes", 1)) {
            for (int i = 0; i < 5; ++i) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::RadioButton(follow_labels[i], follow_mode == i)) {
                    follow_mode = i;
                }
            }
            ImGui::EndTable();
        }
        if (follow_mode != state.follow_mode) {
            state.follow_mode = follow_mode;
            game.set_follow_selected(follow_mode == 1);
            game.set_follow_oldest_largest(follow_mode == 2);
            game.set_follow_oldest_smallest(follow_mode == 3);
            game.set_follow_oldest_middle(follow_mode == 4);
        }
        show_hover_text("Choose one follow target; modes are mutually exclusive.");
        if (const auto* followed = game.get_follow_target_creature()) {
            ImGui::Separator();
            ImGui::Text("Followed creature");
            ImGui::Text("Age: %.2fs  Generation: %d", game.get_sim_time() - followed->get_creation_time(), followed->get_generation());
            ImGui::Text("Area: %.3f  Radius: %.3f", followed->getArea(), followed->getRadius());
            const neat::Genome& brain = followed->get_brain();
            ImGui::Text("Nodes: %zu  Connections: %zu", brain.nodes.size(), brain.connections.size());
            render_brain_graph(brain);
        }
        const neat::Genome* selected_brain = game.get_selected_brain();
        int selected_gen = game.get_selected_generation();
        if (selected_brain) {
            ImGui::Separator();
            ImGui::Text("Selected creature: generation %d", selected_gen);
            ImGui::Text("Nodes: %zu", selected_brain->nodes.size());
            ImGui::Text("Connections: %zu", selected_brain->connections.size());
            if (const auto* creature = game.get_selected_creature()) {
                ImGui::Text("Age: %.2fs", game.get_sim_time() - creature->get_creation_time());
                ImGui::Text("Area: %.3f  Radius: %.3f", creature->getArea(), creature->getRadius());
            }

            render_brain_graph(*selected_brain);
        } else {
            ImGui::Separator();
            ImGui::Text("No creature selected");
        }
    }

    ImGui::EndTabItem();
}

void render_simulation_tab(Game& game, UiState& state) {
    if (!ImGui::BeginTabItem("Simulation")) {
        return;
    }

    if (ImGui::CollapsingHeader("Brain update rate", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Creature brain update per sim second", &state.brain_updates_per_sim_second, 0.1f, 60.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            game.set_brain_updates_per_sim_second(state.brain_updates_per_sim_second);
        }
        show_hover_text("How many times creature AI brains tick per simulated second.");
    }

    if (ImGui::CollapsingHeader("Sizes & costs", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::SliderFloat("Minimum creature area (m^2)", &state.minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            game.set_minimum_area(state.minimum_area);
        }
        show_hover_text("Smallest allowed size before circles are considered too tiny to exist.");

        if (ImGui::SliderFloat("Creature spawn area (m^2)", &state.average_creature_area, 0.1f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            game.set_average_creature_area(state.average_creature_area);
        }
        show_hover_text("Area given to newly created creature circles.");

        if (ImGui::SliderFloat("Food pellet area (m^2)", &state.eatable_area, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            game.set_add_eatable_area(state.eatable_area);
        }
        show_hover_text("Area given to each food pellet you add or drag out.");
        if (ImGui::SliderFloat("Boost cost (m^2)", &state.boost_area, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            game.set_boost_area(state.boost_area);
        }
        show_hover_text("Area a creature spends to dash forward; 0 means no pellet is left behind. Finer range.");
    }

    if (ImGui::CollapsingHeader("Impulse & damping", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool movement_changed = false;
        ImGui::Separator();
        movement_changed |= ImGui::SliderFloat("Forward impulse", &state.linear_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Force applied when brains choose to move straight ahead.");
        movement_changed |= ImGui::SliderFloat("Turn impulse", &state.angular_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::Separator();
        show_hover_text("Strength of turning pulses from AI decisions.");
        movement_changed |= ImGui::SliderFloat("Linear damping", &state.linear_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How quickly forward motion bleeds off (like friction).");
        movement_changed |= ImGui::SliderFloat("Angular damping", &state.angular_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How quickly spinning slows down.");
        ImGui::Separator();
        if (ImGui::SliderFloat("Boost particle impulse fraction", &state.boost_particle_impulse_fraction, 0.0f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic)) {
            game.set_boost_particle_impulse_fraction(state.boost_particle_impulse_fraction);
        }
        show_hover_text("Fraction of the creature's impulse given to the spawned boost particle (fine range).");
        if (ImGui::SliderFloat("Boost particle linear damping", &state.boost_particle_linear_damping, 0.1f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
            game.set_boost_particle_linear_damping(state.boost_particle_linear_damping);
        }
        show_hover_text("Linear damping applied to boost particles only (broader range).");

        if (movement_changed) {
            game.set_circle_density(state.circle_density);
            game.set_linear_impulse_magnitude(state.linear_impulse);
            game.set_angular_impulse_magnitude(state.angular_impulse);
            game.set_linear_damping(state.linear_damping);
            game.set_angular_damping(state.angular_damping);
        }
    }

    if (ImGui::CollapsingHeader("Death & division", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SeparatorText("Death");
        if (ImGui::SliderFloat("Toxic pellet death prob", &state.poison_death_probability, 0.0f, 1.0f, "%.2f")) {
            game.set_poison_death_probability(state.poison_death_probability);
        }
        show_hover_text("Chance that eating a toxic pellet kills a creature.");
        if (ImGui::SliderFloat("Food pellet death prob", &state.poison_death_probability_normal, 0.0f, 1.0f, "%.2f")) {
            game.set_poison_death_probability_normal(state.poison_death_probability_normal);
        }
        show_hover_text("Baseline toxic lethality when circles are not boosted.");
        if (ImGui::SliderFloat("Death Remain Area %", &state.creature_cloud_area_percentage, 0.0f, 100.0f, "%.0f")) {
            game.set_creature_cloud_area_percentage(state.creature_cloud_area_percentage);
        }
        show_hover_text("Percent of a creature's area that returns as pellets when it dies to poison.");
        if (ImGui::SliderFloat("Inactivity timeout (s)", &state.inactivity_timeout, 0.0f, 60.0f, "%.1f")) {
            game.set_inactivity_timeout(state.inactivity_timeout);
        }
        show_hover_text("If a creature fails to boost forward for this many seconds, it dies like poison.");

        ImGui::SeparatorText("Division");
        if (ImGui::SliderFloat("Division pellet divide prob", &state.division_pellet_divide_probability, 0.0f, 1.0f, "%.2f")) {
            game.set_division_pellet_divide_probability(state.division_pellet_divide_probability);
        }
        show_hover_text("Probability a creature divides after eating a blue division pellet.");
    }

    ImGui::EndTabItem();
}

void render_mutation_tab(Game& game, UiState& state) {
    if (!ImGui::BeginTabItem("Mutation")) {
        return;
    }

    if (ImGui::CollapsingHeader("Mutation tuning", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SeparatorText("NEAT mutate parameters");
        bool mutate_changed = false;
        mutate_changed |= ImGui::Checkbox("Allow recurrent connections", &state.mutate_allow_recurrent);
        show_hover_text("Passed to NEAT mutate as areRecurrentConnectionsAllowed.");
        mutate_changed |= ImGui::SliderFloat("Weight mutate prob", &state.mutate_weight_thresh, 0.0f, 1.0f, "%.2f");
        show_hover_text("mutateWeightThresh: probability to mutate a weight.");
        mutate_changed |= ImGui::SliderFloat("Weight full-change prob", &state.mutate_weight_full_change_thresh, 0.0f, 1.0f, "%.2f");
        show_hover_text("mutateWeightFullChangeThresh: chance a weight is completely reassigned.");
        mutate_changed |= ImGui::SliderFloat("Weight factor", &state.mutate_weight_factor, 0.0f, 3.0f, "%.2f");
        show_hover_text("mutateWeightFactor: scale factor for perturbations.");
        mutate_changed |= ImGui::SliderInt("Max iter find connection", &state.mutate_add_connection_iterations, 1, 100);
        show_hover_text("maxIterationsFindConnectionThresh passed to mutate.");
        mutate_changed |= ImGui::SliderFloat("Reactivate connection prob", &state.mutate_reactivate_connection_thresh, 0.0f, 1.0f, "%.2f");
        show_hover_text("reactivateConnectionThresh: chance to re-enable a disabled connection.");
        mutate_changed |= ImGui::SliderInt("Max iter find node", &state.mutate_add_node_iterations, 1, 100);
        show_hover_text("maxIterationsFindNodeThresh passed to mutate.");
        if (mutate_changed) {
            game.set_mutate_allow_recurrent(state.mutate_allow_recurrent);
            game.set_mutate_weight_thresh(state.mutate_weight_thresh);
            game.set_mutate_weight_full_change_thresh(state.mutate_weight_full_change_thresh);
            game.set_mutate_weight_factor(state.mutate_weight_factor);
            game.set_mutate_add_connection_iterations(state.mutate_add_connection_iterations);
            game.set_mutate_reactivate_connection_thresh(state.mutate_reactivate_connection_thresh);
            game.set_mutate_add_node_iterations(state.mutate_add_node_iterations);
        }

        ImGui::SeparatorText("Division mutation (matches NEAT mutate)");
        bool division_mutate_changed = false;
        division_mutate_changed |= ImGui::SliderFloat("Add node % (mutate add node)", &state.add_node_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Probability passed to NEAT mutate for adding a node during division.");
        division_mutate_changed |= ImGui::SliderFloat("Add connection % (mutate add link)", &state.add_connection_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Probability passed to NEAT mutate for adding a connection during division.");
        division_mutate_changed |= ImGui::SliderInt("Mutation rounds", &state.mutation_rounds, 0, 50);
        show_hover_text("How many times to roll the mutation probabilities when a creature divides.");
        if (division_mutate_changed) {
            game.set_add_node_probability(state.add_node_probability);
            game.set_add_connection_probability(state.add_connection_probability);
            game.set_mutation_rounds(state.mutation_rounds);
        }

        ImGui::SeparatorText("Live mutation (matches NEAT mutate)");
        if (ImGui::Checkbox("Enable live mutation", &state.live_mutation_enabled)) {
            game.set_live_mutation_enabled(state.live_mutation_enabled);
        }
        show_hover_text("When off, no per-tick brain mutations happen. Off by default.");
        ImGui::BeginDisabled(!state.live_mutation_enabled);
        bool live_mutate_changed = false;
        live_mutate_changed |= ImGui::SliderFloat("Live add node %", &state.tick_add_node_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Chance a creature adds a brain node each behavior tick.");
        live_mutate_changed |= ImGui::SliderFloat("Live add connection %", &state.tick_add_connection_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Chance a creature adds a brain connection each behavior tick.");
        if (live_mutate_changed) {
            game.set_tick_add_node_probability(state.tick_add_node_probability);
            game.set_tick_add_connection_probability(state.tick_add_connection_probability);
        }
        ImGui::EndDisabled();

        ImGui::SeparatorText("Initialization mutation (matches NEAT mutate)");
        bool init_mutate_changed = false;
        init_mutate_changed |= ImGui::SliderFloat("Init add node %", &state.init_add_node_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Probability passed to NEAT mutate for adding a node during initial seeding.");
        init_mutate_changed |= ImGui::SliderFloat("Init add connection %", &state.init_add_connection_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Probability passed to NEAT mutate for adding a connection during initial seeding.");
        init_mutate_changed |= ImGui::SliderInt("Init mutation rounds", &state.init_mutation_rounds, 0, 100);
        show_hover_text("How many initialization iterations to perform when a creature is created.");
        if (init_mutate_changed) {
            game.set_init_add_node_probability(state.init_add_node_probability);
            game.set_init_add_connection_probability(state.init_add_connection_probability);
            game.set_init_mutation_rounds(state.init_mutation_rounds);
        }
    }

    ImGui::EndTabItem();
}

void render_spawning_tab(Game& game, UiState& state) {
    if (!ImGui::BeginTabItem("Spawning")) {
        return;
    }

    if (ImGui::CollapsingHeader("Spawn & density targets", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool spawning_changed = false;
        spawning_changed |= ImGui::SliderInt("Minimum creature count", &state.minimum_creatures, 0, 500);
        show_hover_text("The simulation auto-spawns new creatures until this count is reached.");
        spawning_changed |= ImGui::SliderFloat("Food area density (m^2 per m^2)", &state.food_density, 0.0f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Target area fraction for non-toxic pellets; the system adjusts spawn/cleanup automatically.");
        spawning_changed |= ImGui::SliderFloat("Toxic area density (m^2 per m^2)", &state.toxic_density, 0.0f, 0.02f, "%.4f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Target area fraction for toxic pellets.");
        spawning_changed |= ImGui::SliderFloat("Division area density (m^2 per m^2)", &state.division_density, 0.0f, 0.02f, "%.4f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Target area fraction for division-triggering blue pellets.");
        if (spawning_changed) {
            game.set_minimum_creature_count(state.minimum_creatures);
            game.set_food_pellet_density(state.food_density);
            game.set_toxic_pellet_density(state.toxic_density);
            game.set_division_pellet_density(state.division_density);
        }
    }

    if (ImGui::CollapsingHeader("Cleanup & utilities", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Remove random %", &state.delete_percentage, 0.0f, 100.0f, "%.1f");
        show_hover_text("Percent of all circles to delete at random when the button is pressed.");
        if (ImGui::Button("Cull random circles")) {
            game.remove_random_percentage(state.delete_percentage);
        }
        show_hover_text("Deletes a random selection of circles using the percentage above.");
        ImGui::SeparatorText("Cleanup pellets (max targets)");
        bool pellet_limits_changed = false;
        pellet_limits_changed |= ImGui::SliderInt("Max food pellets", &state.max_food_pellets, 0, 1000);
        pellet_limits_changed |= ImGui::SliderInt("Max toxic pellets", &state.max_toxic_pellets, 0, 1000);
        pellet_limits_changed |= ImGui::SliderInt("Max division pellets", &state.max_division_pellets, 0, 1000);
        show_hover_text("System auto-adjusts cleanup rates to keep pellets near these targets.");
        if (pellet_limits_changed) {
            game.set_max_food_pellets(state.max_food_pellets);
            game.set_max_toxic_pellets(state.max_toxic_pellets);
            game.set_max_division_pellets(state.max_division_pellets);
        }
    }

    ImGui::EndTabItem();
}
} // namespace

void render_ui(sf::RenderWindow& window, sf::View& view, Game& game) {
    static UiState state;
    initialize_state(state, game);

    ImGui::Begin("Simulation Controls");

    render_view_controls(window, view, game, state);

    ImGui::SeparatorText("Cursor mode");
    render_cursor_controls(game, state);

    ImGui::SeparatorText("Simulation control");
    render_simulation_controls(game, state);

    ImGui::SeparatorText("Spawning region");
    render_spawning_region(game, state);

    ImGui::SeparatorText("Quick presets");
    render_preset_buttons(game, state);

    ImGui::Separator();

    if (ImGui::BeginTabBar("ControlsTabs")) {
        render_overview_tab(game, state);
        render_simulation_tab(game, state);
        render_mutation_tab(game, state);
        render_spawning_tab(game, state);
        ImGui::EndTabBar();
    }
    ImGui::End();
}
