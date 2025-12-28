#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <print>
#include <string>
#include <vector>

constexpr float HSTEP = 13;
constexpr float VSTEP = 18;
constexpr float SCROLL_STEP = 100;
constexpr int FONT_SIZE = 16;

std::vector<draw_text> layout(std::string_view text) {
    std::vector<draw_text> display_list;
    sf::String unicode_text = sf::String::fromUtf8(text.begin(), text.end());

    float cursor_x = HSTEP;
    float cursor_y = VSTEP;
    for (char32_t codepoint : unicode_text) {
        sf::String single_char(codepoint);
        display_list.push_back({cursor_x, cursor_y, single_char});

        cursor_x += HSTEP;
        if (cursor_x >= WIDTH - HSTEP) {
            cursor_y += VSTEP;
            cursor_x = HSTEP;
        }
    }

    return display_list;
}

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    // NOTE: This only works on my computer; Add this to assets.
    if (!font_.openFromFile("/usr/share/fonts/google-noto-sans-cjk-vf-fonts/NotoSansCJK-VF.ttc")) {
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
    if (!result) return;

    std::string raw_utf8 = lex(result.value());
    display_list_ = layout(raw_utf8);
}

void browser::run() {
    while (window_.isOpen()) {
        while (const std::optional event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window_.close();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) scroll_ += SCROLL_STEP;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) scroll_ -= SCROLL_STEP;

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
    }
}
