#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <string>

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
    sf::String unicode_text = sf::String::fromUtf8(raw_utf8.begin(), raw_utf8.end());

    display_list_.clear();

    constexpr float DISPLAYLIST_X = 100;
    constexpr float DISPLAYLIST_Y = 100;

    for (char32_t codepoint : unicode_text) {
        sf::String single_char(codepoint);
        display_list_.push_back({DISPLAYLIST_X, DISPLAYLIST_Y, single_char});
    }
}

void browser::run() {
    while (window_.isOpen()) {
        while (const std::optional event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window_.close();
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
            label.setFillColor(sf::Color::Black);
            window_.draw(label);

            cursor_x += HSTEP;

            if (cursor_x >= WIDTH - HSTEP) {
                cursor_y += VSTEP;
                cursor_x = HSTEP;
            }
        }

        window_.display();
    }
}
