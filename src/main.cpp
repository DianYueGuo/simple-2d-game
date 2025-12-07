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
