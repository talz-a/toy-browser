#pragma once

#include <SFML/Graphics.hpp>
#include "browser/url.hpp"

class browser {
public:
    browser();

    void load(const url& target_url);

    void run();

private:
    sf::RenderWindow window_;
    static constexpr unsigned int WIDTH = 800;
    static constexpr unsigned int HEIGHT = 600;

    sf::Font font_;
    bool font_loaded_{false};
};

// while (window.isOpen()) {
//     while (const std::optional event = window.pollEvent()) {
//         if (event->is<sf::Event::Closed>()) {
//             window.close();
//         }
//     }
//     window.clear();
//     window.display();
// }