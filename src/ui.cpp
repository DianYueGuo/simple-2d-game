#include <imgui.h>
#include <imgui-SFML.h>

#include "ui.hpp"
#include "eater_circle.hpp"

namespace {
struct UiState {
    int cursor_mode = static_cast<int>(Game::CursorMode::Add);
    int add_type = static_cast<int>(Game::AddType::Eater);
    float eatable_area = 1.0f;
    float delete_percentage = 10.0f;
    float pixel_per_meter = 0.0f;
    float petri_radius = 0.0f;
    float time_scale = 0.0f;
    float brain_updates_per_sim_second = 0.0f;
    float minimum_area = 0.0f;
    float average_eater_area = 0.0f;
    float boost_area = 0.0f;
    float poison_death_probability = 0.0f;
    float poison_death_probability_normal = 0.0f;
    float eater_cloud_area_percentage = 0.0f;
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
    float sprinkle_rate_eater = 0.0f;
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

void show_hover_text(const char* description) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", description);
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
    if (ImGui::RadioButton("Add circles", state.cursor_mode == static_cast<int>(Game::CursorMode::Add))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Add);
        cursor_mode_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Move view", state.cursor_mode == static_cast<int>(Game::CursorMode::Drag))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Drag);
        cursor_mode_changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Select", state.cursor_mode == static_cast<int>(Game::CursorMode::Select))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Select);
        cursor_mode_changed = true;
    }
    show_hover_text("Add mode places new circles; Move view lets you pan the camera.");
    if (cursor_mode_changed) {
        game.set_cursor_mode(static_cast<Game::CursorMode>(state.cursor_mode));
    }

    if (state.cursor_mode == static_cast<int>(Game::CursorMode::Add)) {
        bool add_type_changed = false;
        if (ImGui::RadioButton("Eater (hunter)", state.add_type == static_cast<int>(Game::AddType::Eater))) {
            state.add_type = static_cast<int>(Game::AddType::Eater);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Food pellet", state.add_type == static_cast<int>(Game::AddType::Eatable))) {
            state.add_type = static_cast<int>(Game::AddType::Eatable);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Toxic pellet", state.add_type == static_cast<int>(Game::AddType::ToxicEatable))) {
            state.add_type = static_cast<int>(Game::AddType::ToxicEatable);
            add_type_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Division pellet", state.add_type == static_cast<int>(Game::AddType::DivisionEatable))) {
            state.add_type = static_cast<int>(Game::AddType::DivisionEatable);
            add_type_changed = true;
        }
        show_hover_text("Choose what to place when clicking in Add mode.");
        if (add_type_changed) {
            game.set_add_type(static_cast<Game::AddType>(state.add_type));
        }

        if (ImGui::SliderFloat("Spawned food area", &state.eatable_area, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
            game.set_add_eatable_area(state.eatable_area);
        }
        show_hover_text("Area given to each food pellet you add or drag out.");
    }
}
} // namespace

void render_ui(sf::RenderWindow& window, sf::View& view, Game& game) {
    static UiState state;
    if (!state.initialized) {
        state.cursor_mode = static_cast<int>(game.get_cursor_mode());
        state.add_type = static_cast<int>(game.get_add_type());
        state.eatable_area = game.get_add_eatable_area();
        state.pixel_per_meter = game.get_pixles_per_meter();
        state.petri_radius = game.get_petri_radius();
        state.time_scale = game.get_time_scale();
        state.brain_updates_per_sim_second = game.get_brain_updates_per_sim_second();
        state.minimum_area = game.get_minimum_area();
        state.average_eater_area = game.get_average_eater_area();
        state.boost_area = game.get_boost_area();
        state.poison_death_probability = game.get_poison_death_probability();
        state.poison_death_probability_normal = game.get_poison_death_probability_normal();
        state.eater_cloud_area_percentage = game.get_eater_cloud_area_percentage();
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
        state.sprinkle_rate_eater = game.get_sprinkle_rate_eater();
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

    ImGui::Begin("Simulation Controls");
    if (ImGui::BeginTabBar("ControlsTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            ImGui::Text("Active circles: %zu", game.get_circle_count());
            show_hover_text("How many circles currently exist inside the dish.");
            ImGui::Text("Eaters: %zu", game.get_eater_count());
            show_hover_text("Number of eater circles currently alive.");
            bool paused = game.is_paused();
            if (ImGui::Checkbox("Pause simulation", &paused)) {
                game.set_paused(paused);
            }
            show_hover_text("Stop simulation updates so you can inspect selected eater info.");
            ImGui::SeparatorText("Camera follow");
            int follow_mode = state.follow_mode;
            const char* follow_labels[] = {
                "None",
                "Selected eater",
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
            ImGui::Text("Sim time: %.2fs  Real time: %.2fs  FPS: %.1f", game.get_sim_time(), game.get_real_time(), game.get_last_fps());
            show_hover_text("Sim time is the accumulated simulated seconds; real is wall time since start.");
            ImGui::Text("Longest life  creation/division: %.2fs / %.2fs",
                        game.get_longest_life_since_creation(),
                        game.get_longest_life_since_division());
            show_hover_text("Longest survival among eaters since spawn and since their last division.");
            ImGui::Text("Max generation: %d", game.get_max_generation());
            show_hover_text("Highest division count reached by any eater so far.");
            if (const auto* followed = game.get_follow_target_eater()) {
                ImGui::Separator();
                ImGui::Text("Followed eater");
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
                ImGui::Text("Selected eater: generation %d", selected_gen);
                ImGui::Text("Nodes: %zu", selected_brain->nodes.size());
                ImGui::Text("Connections: %zu", selected_brain->connections.size());
                // Show area/radius if we still have the circle
                if (const auto* eater = game.get_selected_eater()) {
                    ImGui::Text("Age: %.2fs", game.get_sim_time() - eater->get_creation_time());
                    ImGui::Text("Area: %.3f  Radius: %.3f", eater->getArea(), eater->getRadius());
                }

                render_brain_graph(*selected_brain);
            } else {
                ImGui::Separator();
                ImGui::Text("No eater selected");
            }

            ImGui::Separator();
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
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("World")) {
            if (ImGui::SliderFloat("World scale (px per m)", &state.pixel_per_meter, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_pixles_per_meter(state.pixel_per_meter);
            }
            show_hover_text("Rendering scale only; higher values zoom in without altering physics.");

            if (ImGui::SliderFloat("Dish radius (m)", &state.petri_radius, 1.0f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_petri_radius(state.petri_radius);
            }
            show_hover_text("Size of the petri dish in world meters.");

            bool auto_remove_outside = game.get_auto_remove_outside();
            if (ImGui::Checkbox("Auto-remove outside dish", &auto_remove_outside)) {
                game.set_auto_remove_outside(auto_remove_outside);
            }
            show_hover_text("Automatically culls any circle that leaves the dish boundary.");

            if (ImGui::Button("Reset view to center")) {
                view = window.getView();
                view.setSize({static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)});
                view.setCenter({0.0f, 0.0f});
                window.setView(view);
            }
            show_hover_text("Recenter and reset the camera zoom.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Simulation")) {
            if (ImGui::SliderFloat("Simulation speed", &state.time_scale, 0.01f, 8.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_time_scale(state.time_scale);
            }
            show_hover_text("Multiplies the physics time step; lower values slow everything down.");

            if (ImGui::SliderFloat("AI updates per sim second", &state.brain_updates_per_sim_second, 0.1f, 60.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_brain_updates_per_sim_second(state.brain_updates_per_sim_second);
            }
            show_hover_text("How many times eater AI brains tick per simulated second.");

            if (ImGui::SliderFloat("Minimum circle area", &state.minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_minimum_area(state.minimum_area);
            }
            show_hover_text("Smallest allowed size before circles are considered too tiny to exist.");

            if (ImGui::SliderFloat("Eater spawn area", &state.average_eater_area, 0.1f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
                game.set_average_eater_area(state.average_eater_area);
            }
            show_hover_text("Area given to newly created eater circles.");

            if (ImGui::SliderFloat("Boost cost (area)", &state.boost_area, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
                game.set_boost_area(state.boost_area);
            }
            show_hover_text("Area an eater spends to dash forward; 0 means no pellet is left behind. Finer range.");
            if (ImGui::SliderFloat("Boost pellet impulse fraction", &state.boost_particle_impulse_fraction, 0.0f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic)) {
                game.set_boost_particle_impulse_fraction(state.boost_particle_impulse_fraction);
            }
            show_hover_text("Fraction of the eater's impulse given to the spawned boost pellet (fine range).");
            if (ImGui::SliderFloat("Boost pellet linear damping", &state.boost_particle_linear_damping, 0.1f, 20.0f, "%.3f", ImGuiSliderFlags_Logarithmic)) {
                game.set_boost_particle_linear_damping(state.boost_particle_linear_damping);
            }
            show_hover_text("Linear damping applied to boost pellets only (broader range).");

            if (ImGui::SliderFloat("Poison cloud area %", &state.eater_cloud_area_percentage, 0.0f, 100.0f, "%.0f")) {
                game.set_eater_cloud_area_percentage(state.eater_cloud_area_percentage);
            }
            show_hover_text("Percent of an eater's area that returns as pellets when it dies to poison.");
            if (ImGui::SliderFloat("Division pellet divide prob", &state.division_pellet_divide_probability, 0.0f, 1.0f, "%.2f")) {
                game.set_division_pellet_divide_probability(state.division_pellet_divide_probability);
            }
            show_hover_text("Probability an eater divides after eating a blue division pellet.");

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
            show_hover_text("How many times to roll the mutation probabilities when an eater divides.");
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
            show_hover_text("Chance an eater adds a brain node each behavior tick.");
            live_mutate_changed |= ImGui::SliderFloat("Live add connection %", &state.tick_add_connection_probability, 0.0f, 1.0f, "%.2f");
            show_hover_text("Chance an eater adds a brain connection each behavior tick.");
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
            show_hover_text("How many initialization iterations to perform when an eater is created.");
            if (init_mutate_changed) {
                game.set_init_add_node_probability(state.init_add_node_probability);
                game.set_init_add_connection_probability(state.init_add_connection_probability);
                game.set_init_mutation_rounds(state.init_mutation_rounds);
            }

            if (ImGui::Checkbox("Show true color (disable smoothing)", &state.show_true_color)) {
                game.set_show_true_color(state.show_true_color);
            }
            show_hover_text("Toggle between smoothed display color and raw brain output color.");

            if (ImGui::SliderFloat("Inactivity timeout (s)", &state.inactivity_timeout, 0.0f, 60.0f, "%.1f")) {
                game.set_inactivity_timeout(state.inactivity_timeout);
            }
            show_hover_text("If an eater fails to boost forward for this many seconds, it dies like poison.");

            if (ImGui::SliderFloat("Toxic death chance", &state.poison_death_probability, 0.0f, 1.0f, "%.2f")) {
                game.set_poison_death_probability(state.poison_death_probability);
            }
            show_hover_text("Chance that eating a toxic pellet kills an eater.");
            if (ImGui::SliderFloat("Toxic death chance (normal)", &state.poison_death_probability_normal, 0.0f, 1.0f, "%.2f")) {
                game.set_poison_death_probability_normal(state.poison_death_probability_normal);
            }
            show_hover_text("Baseline toxic lethality when circles are not boosted.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Movement")) {
            bool movement_changed = false;
            movement_changed |= ImGui::SliderFloat("Circle density", &state.circle_density, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Mass density for all circles; heavier circles resist movement more.");
            movement_changed |= ImGui::SliderFloat("Forward impulse", &state.linear_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Force applied when brains choose to move straight ahead.");
            movement_changed |= ImGui::SliderFloat("Turn impulse", &state.angular_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Strength of turning pulses from AI decisions.");
            movement_changed |= ImGui::SliderFloat("Linear damping", &state.linear_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How quickly forward motion bleeds off (like friction).");
            movement_changed |= ImGui::SliderFloat("Angular damping", &state.angular_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How quickly spinning slows down.");

            if (movement_changed) {
                game.set_circle_density(state.circle_density);
                game.set_linear_impulse_magnitude(state.linear_impulse);
                game.set_angular_impulse_magnitude(state.angular_impulse);
                game.set_linear_damping(state.linear_damping);
                game.set_angular_damping(state.angular_damping);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Spawning")) {
            bool spawning_changed = false;
            spawning_changed |= ImGui::SliderFloat("Eater spawn rate (per s)", &state.sprinkle_rate_eater, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How many eater circles are added each second.");
            spawning_changed |= ImGui::SliderFloat("Food area density (m^2 per m^2)", &state.food_density, 0.0f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Target area fraction for non-toxic pellets; the system adjusts spawn/cleanup automatically.");
            spawning_changed |= ImGui::SliderFloat("Toxic area density (m^2 per m^2)", &state.toxic_density, 0.0f, 0.02f, "%.4f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Target area fraction for toxic pellets.");
            spawning_changed |= ImGui::SliderFloat("Division area density (m^2 per m^2)", &state.division_density, 0.0f, 0.02f, "%.4f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Target area fraction for division-triggering blue pellets.");
            if (spawning_changed) {
                game.set_sprinkle_rate_eater(state.sprinkle_rate_eater);
                game.set_food_pellet_density(state.food_density);
                game.set_toxic_pellet_density(state.toxic_density);
                game.set_division_pellet_density(state.division_density);
            }
            ImGui::Text("Current pellets - food: %zu  toxic: %zu  division: %zu",
                        game.get_food_pellet_count(),
                        game.get_toxic_pellet_count(),
                        game.get_division_pellet_count());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Cursor & Tools")) {
            render_cursor_controls(game, state);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}
