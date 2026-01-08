#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
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
            if (c == '\n' || c == '\t') {
                text += ' ';
            } else {
                text += c;
            }
        }
    }
    return text;
}

void browser::layout(std::string_view text) {
    display_list_.clear();

    float cursor_x = HSTEP;
    float cursor_y = VSTEP;

    // TODO: This should probably be moved to constant.
    sf::Text space(font_, ' ', FONT_SIZE);
    const float space_width = space.getGlobalBounds().size.x;

    for (const auto w : std::views::split(text, ' ')) {
        if (w.empty()) continue;

        sf::Text word(font_, std::string(std::string_view(w)), FONT_SIZE);
        const auto word_with = word.getGlobalBounds().size.x;

        if (cursor_x + word_with > WIDTH - HSTEP) {
            cursor_y += font_.getLineSpacing(FONT_SIZE) * 1.25f;
            cursor_x = HSTEP;
        }

        display_list_.push_back({cursor_x, cursor_y, word});

        cursor_x += word_with + space_width;
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

    for (auto& [x, y, word] : display_list_) {
        if (y > scroll_ + HEIGHT) continue;
        if (y + VSTEP < scroll_) continue;
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
