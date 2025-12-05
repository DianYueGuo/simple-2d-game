#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "game.hpp"

#include <time.h>


int main() {
    srand(time(NULL));

    Game game;

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Petri Dish Simulation");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;
    sf::View view = window.getDefaultView();
    while (window.isOpen())
    {
        game.process_game_logic();

        while (const auto event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            if (const auto* resize = event->getIf<sf::Event::Resized>()) {
                const sf::Vector2f current_center = view.getCenter();
                view.setSize({static_cast<float>(resize->size.x), static_cast<float>(resize->size.y)});
                view.setCenter(current_center);
                window.setView(view);
            }

            if (ImGui::GetIO().WantCaptureMouse)
                continue;

            game.process_input_events(window, event);
        }

        // Sync local view with any changes made during input handling (pan/zoom).
        view = window.getView();

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Simulation Controls");

        static float pixel_per_meter = 15.0f;
        ImGui::SliderFloat("Pixel Per Meter", &pixel_per_meter, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_pixles_per_meter(pixel_per_meter);

        static float time_scale = 1.0f;
        ImGui::SliderFloat("Time Scale", &time_scale, 0.01f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_time_scale(time_scale);

        static float brain_updates_per_sim_second = 10.0f;
        ImGui::SliderFloat("Brain Updates / Sim Second", &brain_updates_per_sim_second, 0.1f, 240.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_brain_updates_per_sim_second(brain_updates_per_sim_second);

        static float minimum_area = 1.0f;
        ImGui::SliderFloat("Minimum Area", &minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_minimum_area(minimum_area);

        static float boost_area = 0.3f;
        ImGui::SliderFloat("Boost Area", &boost_area, 0.01f, 3.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_boost_area(boost_area);

        static int cursor_mode = static_cast<int>(Game::CursorMode::Add);
        if (ImGui::RadioButton("Add", cursor_mode == static_cast<int>(Game::CursorMode::Add))) {
            cursor_mode = static_cast<int>(Game::CursorMode::Add);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Drag", cursor_mode == static_cast<int>(Game::CursorMode::Drag))) {
            cursor_mode = static_cast<int>(Game::CursorMode::Drag);
        }
        game.set_cursor_mode(static_cast<Game::CursorMode>(cursor_mode));

        if (cursor_mode == static_cast<int>(Game::CursorMode::Add)) {
            static int add_type = static_cast<int>(Game::AddType::Eater);
            if (ImGui::RadioButton("Eater", add_type == static_cast<int>(Game::AddType::Eater))) {
                add_type = static_cast<int>(Game::AddType::Eater);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Eatable", add_type == static_cast<int>(Game::AddType::Eatable))) {
                add_type = static_cast<int>(Game::AddType::Eatable);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Toxic", add_type == static_cast<int>(Game::AddType::ToxicEatable))) {
                add_type = static_cast<int>(Game::AddType::ToxicEatable);
            }
            game.set_add_type(static_cast<Game::AddType>(add_type));

            static float eatable_area = 1.0f;
            ImGui::SliderFloat("Eatable Area", &eatable_area, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            game.set_add_eatable_area(eatable_area);
        }

        static float poison_death_probability = 1.0f;
        ImGui::SliderFloat("Poison Death Probability", &poison_death_probability, 0.0f, 1.0f, "%.2f");
        game.set_poison_death_probability(poison_death_probability);

        ImGui::End();

        window.clear();

        window.setView(view);
        game.draw(window);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}
