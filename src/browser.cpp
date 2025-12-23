#include "browser/browser.hpp"
#include <iostream>

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    if (!font_.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cerr << "Warning: Could not load font. Text will not render.\n";
    } else {
        font_loaded_ = true;
    }
}

void browser::load(const url& target_url) {
    // std::string body = target_url.request();
}

void browser::run() {
    while (window_.isOpen()) {
        while (const std::optional event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window_.close();
            }
        }

        window_.clear(sf::Color::White);

        sf::RectangleShape rect({390.f, 280.f});
        rect.setPosition({10.f, 20.f});
        rect.setOutlineColor(sf::Color::Black);
        rect.setOutlineThickness(1.f);
        rect.setFillColor(sf::Color::Transparent);
        window_.draw(rect);

        sf::CircleShape oval(25.f);  // radius
        oval.setPosition({100.f, 100.f});
        oval.setOutlineColor(sf::Color::Black);
        oval.setOutlineThickness(1.f);
        window_.draw(oval);

        if (font_loaded_) {
            sf::Text text(font_, "Hi!", 20);
            text.setPosition({200.f, 150.f});
            text.setFillColor(sf::Color::Black);
            window_.draw(text);
        }

        window_.display();
    }
}