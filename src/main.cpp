#include <iostream>

#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <box2d/box2d.h>

#include "game.hpp"
#include "ui.hpp"

#include <time.h>

void handle_events(sf::RenderWindow& window, sf::View& view, Game& game);


int main() {
    srand(time(NULL));

    Game game;

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Petri Dish Simulation");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        return -1;

    sf::Clock deltaClock;
    sf::View view = window.getDefaultView();
    float aspect = static_cast<float>(window.getSize().x) / static_cast<float>(window.getSize().y);
    float world_height = game.get_petri_radius() * 2.0f;
    float world_width = world_height * aspect;
    view.setSize({world_width, world_height});
    view.setCenter({0.0f, 0.0f});
    window.setView(view);
    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        game.accumulate_real_time(dt);

        game.process_game_logic();

        handle_events(window, view, game);

        view = window.getView(); // sync view after input handling
        game.update_follow_view(view);
        window.setView(view);
        ImGui::SFML::Update(window, sf::seconds(dt));

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

void handle_events(sf::RenderWindow& window, sf::View& view, Game& game) {
    static sf::Vector2u previous_window_size = window.getSize();
    while (const auto event = window.pollEvent()) {
        ImGui::SFML::ProcessEvent(window, *event);

        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* resize = event->getIf<sf::Event::Resized>()) {
            // Preserve current center/zoom: scale the view size by the pixel change.
            sf::Vector2u old_size = previous_window_size;
            previous_window_size = resize->size;
            if (old_size.x > 0 && old_size.y > 0) {
                float world_per_pixel_x = view.getSize().x / static_cast<float>(old_size.x);
                float world_per_pixel_y = view.getSize().y / static_cast<float>(old_size.y);
                view.setSize({world_per_pixel_x * static_cast<float>(resize->size.x),
                              world_per_pixel_y * static_cast<float>(resize->size.y)});
            }
            window.setView(view);
        }

        if (ImGui::GetIO().WantCaptureMouse) {
            continue;
        }

        game.process_input_events(window, event);
    }
}
