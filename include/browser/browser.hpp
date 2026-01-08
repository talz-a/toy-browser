#pragma once

#include <SFML/Graphics.hpp>
#include <string_view>
#include <vector>
#include "browser/url.hpp"

struct render_item {
    float x{}, y{};
    sf::Text text;
};

struct text_token {
    std::string text;
};

struct tag_token {
    std::string tag;
};

using token = std::variant<text_token, tag_token>;

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

    [[nodiscard]] static std::vector<token> lex(std::string_view body);
    void layout(const std::vector<token>& toks);
    void process_events();
    void render();

    float scroll_ = 0;

    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<render_item> display_list_;
};
