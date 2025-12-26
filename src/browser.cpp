#include "browser/browser.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>
#include <print>

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    if (!font_.openFromFile("/System/Library/Fonts/SFNS.ttf")) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

std::string browser::lex(std::string_view body) {
    std::string text;
    bool in_tag = false;
    for (char c : body) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            text += c;
        }
    }

    return text;
}

void browser::load(const url& target_url) {
    auto result = target_url.request();

    if (!result) {
        std::println(stderr, "ERROR: {}", static_cast<int>(result.error()));
        return;
    }

    std::string text = lex(result.value());

    display_list_.clear();
    for (char c : text) {
        display_list_.push_back({100.0F, 100.0F, std::string(1, c)});
    }
}

void browser::run() {
    while (window_.isOpen()) {
        while (const std::optional event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window_.close();
            }
        }

        window_.clear(sf::Color::White);

        constexpr float HSTEP = 13;
        constexpr float VSTEP = 18;
        constexpr int FONT_SIZE = 16;
        float cursor_x = HSTEP;
        float cursor_y = VSTEP;
        for (const auto& cmd : display_list_) {
            sf::Text label(font_, cmd.text, FONT_SIZE);
            label.setPosition({cursor_x, cursor_y});
            cursor_x += HSTEP;

            if (cursor_x >= WIDTH - HSTEP) {
                cursor_y += VSTEP;
                cursor_x = HSTEP;
            }

            label.setFillColor(sf::Color::Black);
            window_.draw(label);
        }

        window_.display();
    }
}
