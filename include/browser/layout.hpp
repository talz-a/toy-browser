#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "browser/constants.hpp"
#include "browser/html_parser.hpp"

struct render_item {
    float x{}, y{};
    sf::Text text;
};

struct line_item {
    float x{};
    sf::Text text;
};

class layout {
public:
    layout(const std::shared_ptr<node>& node, sf::Font& font);

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }

    void flush();

private:
    [[nodiscard]] float get_ascent(const sf::Font& font, unsigned int size);

    [[nodiscard]] float get_descent(const sf::Font& font, unsigned int size);

    std::vector<line_item> line_;
    std::vector<render_item> display_list_;
    float cursor_x_ = constants::h_step;
    float cursor_y_ = constants::v_step;
    int weight_ = sf::Text::Style::Regular;
    int style_ = sf::Text::Style::Regular;
    int size_ = constants::font_size;

    // TODO: Change this.
    const sf::Font* font_;

    void recurse(const std::shared_ptr<node>& node);

    void open_tag(const element_data& element);
    void close_tag(const element_data& element);

    void word(const std::string& word);
};
