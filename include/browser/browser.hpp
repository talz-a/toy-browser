#pragma once

#include <SFML/Graphics.hpp>
#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <browser/url.hpp>
#include <expected>

struct Browser {
    Browser();

    std::expected<void, std::string> load(const Url& target_url);

    void run();
    void process_events();
    void render();

    float scroll_ = 0.0f;
    std::optional<DocumentLayout> document_;
    sf::RenderWindow window_;
    sf::Font font_;
    std::vector<draw_cmds> display_list_;
    std::unique_ptr<HTMLNode> nodes_;
};
