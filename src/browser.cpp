#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include "browser/html.hpp"

browser::browser()
    : window_{sf::VideoMode({constants::width, constants::height}), constants::window_title} {
    if (!font_.openFromFile(constants::default_font_path)) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

void browser::load(const url& target_url) {
    auto toks = lex(target_url.request());

    // For testing...
    // std::string test_html =
    //     "Normal <big>Big</big> Normal <small>Small</small> <b>Bold <big>BigBold</big></b>";
    // auto toks = lex(test_html);

    display_list_ = layout(toks, font_).get_display_list();
    run();
}

void browser::process_events() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                scroll_ += constants::scroll_step;
            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ -= constants::scroll_step;
            }
        }
    }
}

void browser::render() {
    window_.clear(sf::Color::White);

    for (auto& [x, y, word] : display_list_) {
        if (y > scroll_ + constants::height) continue;
        if (y + constants::v_step < scroll_) continue;
        word.setPosition({x, y - scroll_});
        word.setFillColor(sf::Color::Black);
        window_.draw(word);
    }

    window_.display();
}

void browser::run() {
    while (window_.isOpen()) {
        process_events();
        render();
    }
}
