#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

browser::browser() : window_{sf::VideoMode({WIDTH, HEIGHT}), "Toy Browser"} {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

std::vector<token> browser::lex(std::string_view body) {
    std::vector<token> out;
    std::string buffer;
    bool in_tag = false;

    for (char c : body) {
        if (c == '<') {
            in_tag = true;
            if (!buffer.empty()) out.emplace_back(text_token{.text = std::move(buffer)});
            buffer.clear();
        } else if (c == '>') {
            in_tag = false;
            out.emplace_back(tag_token{.tag = std::move(buffer)});
            buffer.clear();
        } else {
            // NOTE: Maybe handle the ending '/' of closing tags.
            if (c == '\n' || c == '\t') {
                buffer += ' ';
            } else {
                buffer += c;
            }
        }
    }

    if (!in_tag && !buffer.empty()) out.emplace_back(text_token{.text = std::move(buffer)});
    return out;
}

void browser::load(const url& target_url) {
    // auto result = target_url.request();
    // if (!result) return;
    // auto toks = lex(result.value());

    // For testing...
    std::string test_html =
        "Normal <big>Big</big> Normal <small>Small</small> <b>Bold <big>BigBold</big></b>";
    auto toks = lex(test_html);

    display_list_ = layout(toks, font_).get_display_list();
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
        if (y + layout::VSTEP < scroll_) continue;
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
