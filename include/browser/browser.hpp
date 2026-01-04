#pragma once

#include <SFML/Graphics.hpp>
#include <string_view>
#include <vector>
#include "browser/url.hpp"

struct render_item {
    float x{}, y{};
    sf::String text;
};

class browser {
public:
    static constexpr unsigned int WIDTH = 800;
    static constexpr unsigned int HEIGHT = 600;

    browser();

    void load(const url& target_url);

    void run();

private:
    static constexpr float HSTEP = 13;
    static constexpr float VSTEP = 18;
    static constexpr float SCROLL_STEP = 100;
    static constexpr int FONT_SIZE = 16;

    [[nodiscard]] static std::string lex(std::string_view body);
    void layout(std::string_view text);
    void process_events();
    void render();

    float scroll_ = 0;

    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<render_item> display_list_;
};
