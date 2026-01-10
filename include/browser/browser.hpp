#pragma once

#include <SFML/Graphics.hpp>
#include <string_view>
#include "browser/layout.hpp"
#include "browser/url.hpp"

class browser {
public:
    static constexpr unsigned int WIDTH = 800;
    static constexpr unsigned int HEIGHT = 600;

    browser();

    void load(const url& target_url);

    void run();

private:
    static constexpr float SCROLL_STEP = 100;

    [[nodiscard]] static std::vector<token> lex(std::string_view body);
    void process_events();
    void render();

    float scroll_ = 0;

    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<render_item> display_list_;
};
