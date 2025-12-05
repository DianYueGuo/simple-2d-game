#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "game.hpp"

#include <time.h>

void handle_events(sf::RenderWindow& window, sf::View& view, Game& game);
void render_ui(sf::RenderWindow& window, sf::View& view, Game& game);
void render_cursor_controls(Game& game);
void show_hover_text(const char* description);


int main() {
    srand(time(NULL));

    Game game;

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Petri Dish Simulation");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;
    sf::View view = window.getDefaultView();
    view.setCenter({0.0f, 0.0f});
    window.setView(view);
    while (window.isOpen()) {
        game.process_game_logic();

        handle_events(window, view, game);

        view = window.getView(); // sync view after input handling
        ImGui::SFML::Update(window, deltaClock.restart());

        render_ui(window, view, game);

        window.clear();
        window.setView(view);
        game.draw(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}

void show_hover_text(const char* description) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", description);
    }
}

void handle_events(sf::RenderWindow& window, sf::View& view, Game& game) {
    while (const auto event = window.pollEvent()) {
        ImGui::SFML::ProcessEvent(window, *event);

        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* resize = event->getIf<sf::Event::Resized>()) {
            view.setSize({static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)});
            view.setCenter({0.0f, 0.0f});
            window.setView(view);
        }

        if (ImGui::GetIO().WantCaptureMouse) {
            continue;
        }

        game.process_input_events(window, event);
    }
}

void render_ui(sf::RenderWindow& window, sf::View& view, Game& game) {
    ImGui::Begin("Simulation Controls");
    ImGui::Text("Active circles: %zu", game.get_circle_count());
    show_hover_text("How many circles currently exist inside the dish.");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("World Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float pixel_per_meter = 15.0f;
        ImGui::SliderFloat("World scale (px per m)", &pixel_per_meter, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Rendering scale only; higher values zoom in without altering physics.");
        game.set_pixles_per_meter(pixel_per_meter);

        static float petri_radius = 20.0f;
        ImGui::SliderFloat("Dish radius (m)", &petri_radius, 1.0f, 200.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Size of the petri dish in world meters.");
        game.set_petri_radius(petri_radius);

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
    }

    if (ImGui::CollapsingHeader("Simulation Behavior", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float time_scale = 1.0f;
        ImGui::SliderFloat("Simulation speed", &time_scale, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Multiplies the physics time step; lower values slow everything down.");
        game.set_time_scale(time_scale);

        static float brain_updates_per_sim_second = 10.0f;
        ImGui::SliderFloat("AI updates per sim second", &brain_updates_per_sim_second, 0.1f, 240.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How many times eater AI brains tick per simulated second.");
        game.set_brain_updates_per_sim_second(brain_updates_per_sim_second);

        static float minimum_area = 1.0f;
        ImGui::SliderFloat("Minimum circle area", &minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Smallest allowed size before circles are considered too tiny to exist.");
        game.set_minimum_area(minimum_area);

        static float average_eater_area = 1.8f;
        ImGui::SliderFloat("Eater spawn area", &average_eater_area, 0.1f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Area given to newly created eater circles.");
        game.set_average_eater_area(average_eater_area);

        static float boost_area = 0.3f;
        ImGui::SliderFloat("Boost cost (area)", &boost_area, 0.01f, 3.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Area an eater spends to dash forward, leaving a pellet behind.");
        game.set_boost_area(boost_area);

        static float poison_death_probability = 1.0f;
        ImGui::SliderFloat("Toxic death chance", &poison_death_probability, 0.0f, 1.0f, "%.2f");
        show_hover_text("Chance that eating a toxic pellet kills an eater.");
        game.set_poison_death_probability(poison_death_probability);
        static float poison_death_probability_normal = 0.1f;
        ImGui::SliderFloat("Toxic death chance (normal)", &poison_death_probability_normal, 0.0f, 1.0f, "%.2f");
        show_hover_text("Baseline toxic lethality when circles are not boosted.");
        game.set_poison_death_probability_normal(poison_death_probability_normal);
    }

    if (ImGui::CollapsingHeader("Movement Tuning", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float circle_density = 1.0f;
        static float linear_impulse = 5.0f;
        static float angular_impulse = 5.0f;
        static float linear_damping = 0.3f;
        static float angular_damping = 1.0f;

        ImGui::SliderFloat("Circle density", &circle_density, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Mass density for all circles; heavier circles resist movement more.");
        ImGui::SliderFloat("Forward impulse", &linear_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Force applied when brains choose to move straight ahead.");
        ImGui::SliderFloat("Turn impulse", &angular_impulse, 0.01f, 50.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Strength of turning pulses from AI decisions.");
        ImGui::SliderFloat("Linear damping", &linear_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How quickly forward motion bleeds off (like friction).");
        ImGui::SliderFloat("Angular damping", &angular_damping, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How quickly spinning slows down.");

        game.set_circle_density(circle_density);
        game.set_linear_impulse_magnitude(linear_impulse);
        game.set_angular_impulse_magnitude(angular_impulse);
        game.set_linear_damping(linear_damping);
        game.set_angular_damping(angular_damping);
    }

    if (ImGui::CollapsingHeader("Auto-spawn rates", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float sprinkle_rate_eater = 0.0f;
        static float sprinkle_rate_eatable = 0.0f;
        static float sprinkle_rate_toxic = 0.0f;
        ImGui::SliderFloat("Eater spawn rate (per s)", &sprinkle_rate_eater, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("How many eater circles are added each second.");
        ImGui::SliderFloat("Food spawn rate (per s)", &sprinkle_rate_eatable, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Automatic feed rate for non-toxic food pellets.");
        ImGui::SliderFloat("Toxic spawn rate (per s)", &sprinkle_rate_toxic, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Automatic spawn rate for poisonous pellets.");
        game.set_sprinkle_rate_eater(sprinkle_rate_eater);
        game.set_sprinkle_rate_eatable(sprinkle_rate_eatable);
        game.set_sprinkle_rate_toxic(sprinkle_rate_toxic);
    }

    render_cursor_controls(game);

    static float delete_percentage = 10.0f;
    ImGui::Separator();
    ImGui::SliderFloat("Remove random %", &delete_percentage, 0.0f, 100.0f, "%.1f");
    show_hover_text("Percent of all circles to delete at random when the button is pressed.");
    if (ImGui::Button("Cull random circles")) {
        game.remove_random_percentage(delete_percentage);
    }
    show_hover_text("Deletes a random selection of circles using the percentage above.");

    ImGui::End();
}

void render_cursor_controls(Game& game) {
    static int cursor_mode = static_cast<int>(Game::CursorMode::Add);
    if (ImGui::RadioButton("Add circles", cursor_mode == static_cast<int>(Game::CursorMode::Add))) {
        cursor_mode = static_cast<int>(Game::CursorMode::Add);
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Move view", cursor_mode == static_cast<int>(Game::CursorMode::Drag))) {
        cursor_mode = static_cast<int>(Game::CursorMode::Drag);
    }
    show_hover_text("Add mode places new circles; Move view lets you pan the camera.");
    game.set_cursor_mode(static_cast<Game::CursorMode>(cursor_mode));

    if (cursor_mode == static_cast<int>(Game::CursorMode::Add)) {
        static int add_type = static_cast<int>(Game::AddType::Eater);
        if (ImGui::RadioButton("Eater (hunter)", add_type == static_cast<int>(Game::AddType::Eater))) {
            add_type = static_cast<int>(Game::AddType::Eater);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Food pellet", add_type == static_cast<int>(Game::AddType::Eatable))) {
            add_type = static_cast<int>(Game::AddType::Eatable);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Toxic pellet", add_type == static_cast<int>(Game::AddType::ToxicEatable))) {
            add_type = static_cast<int>(Game::AddType::ToxicEatable);
        }
        show_hover_text("Choose what to place when clicking in Add mode.");
        game.set_add_type(static_cast<Game::AddType>(add_type));

        static float eatable_area = 1.0f;
        ImGui::SliderFloat("Spawned food area", &eatable_area, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        show_hover_text("Area given to each food pellet you add or drag out.");
        game.set_add_eatable_area(eatable_area);
    }
}
