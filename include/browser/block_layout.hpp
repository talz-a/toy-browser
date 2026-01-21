#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "browser/constants.hpp"
#include "browser/html_parser.hpp"

class document_layout;
class block_layout;

using layout_parent = std::variant<document_layout*, block_layout*>;

struct render_item {
    float x{}, y{};
    sf::Text text;
};

struct line_item {
    float x{};
    sf::Text text;
};

class block_layout {
public:
    block_layout(
        const node* n,
        layout_parent parent,
        const block_layout* previous,
        const sf::Font& font,
        float width
    )
        : node_{n}, parent_{parent}, previous_{previous}, font_{&font}, width_{width} {}

    void layout();

    void flush();

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }
    [[nodiscard]] float get_height() const { return cursor_y_; }

private:
    [[nodiscard]] float get_ascent(const sf::Font& font, unsigned int size);
    [[nodiscard]] float get_descent(const sf::Font& font, unsigned int size);

    void recurse(const node* node);

    void open_tag(const element_data& element);
    void close_tag(const element_data& element);

    void word(const std::string& word);

    const node* node_;
    layout_parent parent_;
    const block_layout* previous_ = nullptr;
    std::vector<std::unique_ptr<block_layout>> children_;

    std::vector<line_item> line_;
    std::vector<render_item> display_list_;

    float cursor_x_ = constants::h_step;
    float cursor_y_ = constants::v_step;
    int weight_ = sf::Text::Style::Regular;
    int style_ = sf::Text::Style::Regular;
    int size_ = constants::font_size;
    float width_;

    // Stored as pointer to allow assignment/copying.
    const sf::Font* font_;
};
