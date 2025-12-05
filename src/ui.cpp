#include <imgui.h>
#include <imgui-SFML.h>

#include "ui.hpp"

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
    float circle_density = 0.0f;
    float linear_impulse = 0.0f;
    float angular_impulse = 0.0f;
    float linear_damping = 0.0f;
    float angular_damping = 0.0f;
    float sprinkle_rate_eater = 0.0f;
    float sprinkle_rate_eatable = 0.0f;
    float sprinkle_rate_toxic = 0.0f;
    bool initialized = false;
};

void show_hover_text(const char* description) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", description);
    }
}

void render_cursor_controls(Game& game, UiState& state) {
    if (ImGui::RadioButton("Add circles", state.cursor_mode == static_cast<int>(Game::CursorMode::Add))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Add);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Move view", state.cursor_mode == static_cast<int>(Game::CursorMode::Drag))) {
        state.cursor_mode = static_cast<int>(Game::CursorMode::Drag);
    }
    show_hover_text("Add mode places new circles; Move view lets you pan the camera.");
    game.set_cursor_mode(static_cast<Game::CursorMode>(state.cursor_mode));

    if (state.cursor_mode == static_cast<int>(Game::CursorMode::Add)) {
        if (ImGui::RadioButton("Eater (hunter)", state.add_type == static_cast<int>(Game::AddType::Eater))) {
            state.add_type = static_cast<int>(Game::AddType::Eater);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Food pellet", state.add_type == static_cast<int>(Game::AddType::Eatable))) {
            state.add_type = static_cast<int>(Game::AddType::Eatable);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Toxic pellet", state.add_type == static_cast<int>(Game::AddType::ToxicEatable))) {
            state.add_type = static_cast<int>(Game::AddType::ToxicEatable);
        }
        show_hover_text("Choose what to place when clicking in Add mode.");
        game.set_add_type(static_cast<Game::AddType>(state.add_type));

        ImGui::SliderFloat("Spawned food area", &state.eatable_area, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Area given to each food pellet you add or drag out.");
        game.set_add_eatable_area(state.eatable_area);
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
        state.circle_density = game.get_circle_density();
        state.linear_impulse = game.get_linear_impulse_magnitude();
        state.angular_impulse = game.get_angular_impulse_magnitude();
        state.linear_damping = game.get_linear_damping();
        state.angular_damping = game.get_angular_damping();
        state.sprinkle_rate_eater = game.get_sprinkle_rate_eater();
        state.sprinkle_rate_eatable = game.get_sprinkle_rate_eatable();
        state.sprinkle_rate_toxic = game.get_sprinkle_rate_toxic();
        state.initialized = true;
    }

    ImGui::Begin("Simulation Controls");
    if (ImGui::BeginTabBar("ControlsTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            ImGui::Text("Active circles: %zu", game.get_circle_count());
            show_hover_text("How many circles currently exist inside the dish.");

            ImGui::Separator();
            ImGui::SliderFloat("Remove random %", &state.delete_percentage, 0.0f, 100.0f, "%.1f");
            show_hover_text("Percent of all circles to delete at random when the button is pressed.");
            if (ImGui::Button("Cull random circles")) {
                game.remove_random_percentage(state.delete_percentage);
            }
            show_hover_text("Deletes a random selection of circles using the percentage above.");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("World")) {
            ImGui::SliderFloat("World scale (px per m)", &state.pixel_per_meter, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Rendering scale only; higher values zoom in without altering physics.");
            game.set_pixles_per_meter(state.pixel_per_meter);

            ImGui::SliderFloat("Dish radius (m)", &state.petri_radius, 1.0f, 200.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Size of the petri dish in world meters.");
            game.set_petri_radius(state.petri_radius);

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
            ImGui::SliderFloat("Simulation speed", &state.time_scale, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Multiplies the physics time step; lower values slow everything down.");
            game.set_time_scale(state.time_scale);

            ImGui::SliderFloat("AI updates per sim second", &state.brain_updates_per_sim_second, 0.1f, 240.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How many times eater AI brains tick per simulated second.");
            game.set_brain_updates_per_sim_second(state.brain_updates_per_sim_second);

            ImGui::SliderFloat("Minimum circle area", &state.minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Smallest allowed size before circles are considered too tiny to exist.");
            game.set_minimum_area(state.minimum_area);

            ImGui::SliderFloat("Eater spawn area", &state.average_eater_area, 0.1f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Area given to newly created eater circles.");
            game.set_average_eater_area(state.average_eater_area);

            ImGui::SliderFloat("Boost cost (area)", &state.boost_area, 0.01f, 3.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Area an eater spends to dash forward, leaving a pellet behind.");
            game.set_boost_area(state.boost_area);

            ImGui::SliderFloat("Toxic death chance", &state.poison_death_probability, 0.0f, 1.0f, "%.2f");
            show_hover_text("Chance that eating a toxic pellet kills an eater.");
            game.set_poison_death_probability(state.poison_death_probability);
            ImGui::SliderFloat("Toxic death chance (normal)", &state.poison_death_probability_normal, 0.0f, 1.0f, "%.2f");
            show_hover_text("Baseline toxic lethality when circles are not boosted.");
            game.set_poison_death_probability_normal(state.poison_death_probability_normal);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Movement")) {
            ImGui::SliderFloat("Circle density", &state.circle_density, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Mass density for all circles; heavier circles resist movement more.");
            ImGui::SliderFloat("Forward impulse", &state.linear_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Force applied when brains choose to move straight ahead.");
            ImGui::SliderFloat("Turn impulse", &state.angular_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Strength of turning pulses from AI decisions.");
            ImGui::SliderFloat("Linear damping", &state.linear_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How quickly forward motion bleeds off (like friction).");
            ImGui::SliderFloat("Angular damping", &state.angular_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How quickly spinning slows down.");

            game.set_circle_density(state.circle_density);
            game.set_linear_impulse_magnitude(state.linear_impulse);
            game.set_angular_impulse_magnitude(state.angular_impulse);
            game.set_linear_damping(state.linear_damping);
            game.set_angular_damping(state.angular_damping);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Spawning")) {
            ImGui::SliderFloat("Eater spawn rate (per s)", &state.sprinkle_rate_eater, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("How many eater circles are added each second.");
            ImGui::SliderFloat("Food spawn rate (per s)", &state.sprinkle_rate_eatable, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Automatic feed rate for non-toxic food pellets.");
            ImGui::SliderFloat("Toxic spawn rate (per s)", &state.sprinkle_rate_toxic, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            show_hover_text("Automatic spawn rate for poisonous pellets.");
            game.set_sprinkle_rate_eater(state.sprinkle_rate_eater);
            game.set_sprinkle_rate_eatable(state.sprinkle_rate_eatable);
            game.set_sprinkle_rate_toxic(state.sprinkle_rate_toxic);
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
