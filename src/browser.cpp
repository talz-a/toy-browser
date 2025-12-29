#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    if (!font_.openFromFile("assets/NotoSansCJK-VF.ttc")) {
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

void browser::layout(std::string_view text) {
    display_list_.clear();
    sf::String unicode_text = sf::String::fromUtf8(text.begin(), text.end());

    float cursor_x = HSTEP;
    float cursor_y = VSTEP;
    for (char32_t codepoint : unicode_text) {
        sf::String single_char(codepoint);
        display_list_.push_back({cursor_x, cursor_y, single_char});

        cursor_x += HSTEP;
        if (cursor_x >= WIDTH - HSTEP) {
            cursor_y += VSTEP;
            cursor_x = HSTEP;
        }
    }
}

void browser::load(const url& target_url) {
    auto result = target_url.request();
    if (!result) return;
    std::string raw_utf8 = lex(result.value());
    layout(raw_utf8);
    run();
}

void browser::process_events() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                scroll_ += SCROLL_STEP;
            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ -= SCROLL_STEP;
            }
        }
    }
}

void browser::render() {
    window_.clear(sf::Color::White);

    for (const auto& [x, y, c] : display_list_) {
        if (y > scroll_ + HEIGHT) continue;
        if (y + VSTEP < scroll_) continue;
        sf::Text label(font_, c, FONT_SIZE);
        label.setPosition({x, y - scroll_});
        label.setFillColor(sf::Color::Black);
        window_.draw(label);
    }

    window_.display();
}

void browser::run() {
    while (window_.isOpen()) {
        process_events();
        render();
    }
}
