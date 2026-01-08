#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <ranges>
#include <string>
#include <variant>
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

void browser::layout(const std::vector<token>& toks) {
    display_list_.clear();

    float cursor_x = HSTEP;
    float cursor_y = VSTEP;

    // TODO: This should probably be moved to constant.
    sf::Text space(font_, ' ', FONT_SIZE);
    const float space_width = space.getGlobalBounds().size.x;

    auto weight = sf::Text::Style::Regular;
    auto style = sf::Text::Style::Regular;

    auto visitor = [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, text_token>) {
            for (const auto w : std::views::split(arg.text, ' ')) {
                // TODO: Check if this is the correct if check to make.
                if (w.empty()) continue;

                sf::Text word(font_, std::ranges::to<std::string>(w), FONT_SIZE);
                const auto word_with = word.getGlobalBounds().size.x;

                word.setStyle(style | weight);

                if (cursor_x + word_with > WIDTH - HSTEP) {
                    cursor_y += font_.getLineSpacing(FONT_SIZE) * 1.25f;
                    cursor_x = HSTEP;
                }

                display_list_.push_back({cursor_x, cursor_y, word});

                cursor_x += word_with + space_width;
            }
        } else if constexpr (std::is_same_v<T, tag_token>) {
            // TODO: Switch on tag as enum at some point.
            if (arg.tag == "i") {
                style = sf::Text::Style::Italic;
            } else if (arg.tag == "/i") {
                style = sf::Text::Style::Regular;
            } else if (arg.tag == "b") {
                weight = sf::Text::Style::Bold;
            } else if (arg.tag == "/b") {
                weight = sf::Text::Style::Regular;
            }
        }
    };

    for (auto const& tok : toks) {
        std::visit(visitor, tok);
    }
}

void browser::load(const url& target_url) {
    auto result = target_url.request();
    if (!result) return;
    auto toks = lex(result.value());

    // For testing...
    // std::string test_html = "Normal <b>Bold</b> <i>Italic</i> <b><i>BoldItalic</i></b>";
    // auto toks = lex(test_html);

    layout(toks);
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
