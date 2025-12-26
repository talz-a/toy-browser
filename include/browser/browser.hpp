#pragma once

#include <SFML/Graphics.hpp>
#include <string_view>
#include <vector>
#include "browser/url.hpp"

struct draw_text {
    float x, y;
    sf::String text;
};

class browser {
public:
    browser();

    void load(const url& target_url);

    void run();

private:
    [[nodiscard]] static std::string lex(std::string_view body);

    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<draw_text> display_list_;

    static constexpr unsigned int WIDTH = 800;
    static constexpr unsigned int HEIGHT = 600;
};
