#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "browser/constants.hpp"
#include "browser/html.hpp"

struct render_item {
    float x{}, y{};
    sf::Text text;
};

class layout {
public:
    layout(const std::vector<token>& tokens, const sf::Font& font);

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }

private:
    std::vector<render_item> display_list_;
    float cursor_x_ = constants::h_step;
    float cursor_y_ = constants::v_step;
    int weight_ = sf::Text::Style::Regular;
    int style_ = sf::Text::Style::Regular;
    int size_ = constants::font_size;

    // TODO: Change this.
    const sf::Font* font_;

    void process_token(const token& tok);
    void word(const std::string& word);
};