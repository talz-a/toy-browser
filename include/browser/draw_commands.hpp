#pragma once

#include <SFML/Graphics.hpp>

struct draw_rect {
    draw_rect(float x1, float y1, float x2, float y2, sf::Color color)
        : top_{y1}, left_{x1}, bottom_{y2}, right_{x2}, color_{color} {}

    void execute(float scroll, sf::RenderWindow& window);

    float top_;
    float left_;
    float bottom_;
    float right_;
    sf::Color color_;
};

struct draw_text {
    draw_text(float x1, float y1, sf::Text text) : top_{y1}, left_{x1}, text_{std::move(text)} {
        bottom_ = y1 + text_.getLineSpacing();
    }

    void execute(float scroll, sf::RenderWindow& window);

    float top_;
    float left_;
    float bottom_;
    sf::Text text_;
};

using draw_cmds = std::variant<draw_rect, draw_text>;
