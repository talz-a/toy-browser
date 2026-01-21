#pragma once

#include <SFML/Graphics.hpp>
#include "browser/document_layout.hpp"
#include "browser/html_parser.hpp"
#include "browser/url.hpp"

class browser {
public:
    browser();

    void load(const url& target_url);
    void run();

private:
    void process_events();
    void render();

    float scroll_ = 0.0f;
    std::optional<document_layout> document_;
    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<render_item> display_list_;
    std::unique_ptr<node> nodes_;
};
