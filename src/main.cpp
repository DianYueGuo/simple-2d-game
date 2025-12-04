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

    sf::RenderWindow window(sf::VideoMode({640, 480}), "ImGui + SFML = <3");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;
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

            if (ImGui::GetIO().WantCaptureMouse)
                continue;

            game.process_input_events(event);
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");

        static float pixel_per_meter = 1.0f;
        ImGui::SliderFloat("Pixel Per Meter", &pixel_per_meter, 0.1f, 100.0f);
        game.set_pixles_per_meter(pixel_per_meter);

        ImGui::End();

        window.clear();

        game.draw(window);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();

    return 0;
}
