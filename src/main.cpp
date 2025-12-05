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

    sf::RenderWindow window(sf::VideoMode({640, 480}), "Petri Dish Simulation");
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

        static float brain_updates_per_sim_second = 60.0f;
        ImGui::SliderFloat("Brain Updates / Sim Second", &brain_updates_per_sim_second, 0.1f, 240.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_brain_updates_per_sim_second(brain_updates_per_sim_second);

        static float minimum_area = 1.0f;
        ImGui::SliderFloat("Minimum Area", &minimum_area, 0.1f, 5.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        game.set_minimum_area(minimum_area);

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
