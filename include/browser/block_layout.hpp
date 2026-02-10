#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <browser/constants.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <string>
#include <vector>

struct render_item {
    float x{}, y{};
    sf::Text text;
};

struct line_item {
    float x{};
    sf::Text text;
};

enum class layout_mode : std::uint8_t {
    block,
    inline_context,
};

// @TODO: See if we can get around this forward decl stuff.
struct document_layout;
struct block_layout;
using layout_parent = std::variant<document_layout*, block_layout*>;

struct block_layout {
    block_layout(
        const html_node* n,
        layout_parent parent,
        const block_layout* previous,
        const sf::Font& font,
        float width
    )
        : node_{n}, parent_{parent}, previous_{previous}, font_{&font}, width_{width} {}

    void layout();
    void flush();

    [[nodiscard]] std::vector<draw_cmds> paint();

    [[nodiscard]] float get_ascent(const sf::Font& font, unsigned int size) const;
    [[nodiscard]] float get_descent(const sf::Font& font, unsigned int size) const;

    [[nodiscard]] layout_mode get_layout_mode() const;

    void recurse(const html_node* node);

    void open_tag(const element_data& element);
    void close_tag(const element_data& element);

    void word(const std::string& word);

    const html_node* node_;
    layout_parent parent_;
    const block_layout* previous_ = nullptr;
    float width_;

    std::vector<std::unique_ptr<block_layout>> children_;
    std::vector<line_item> line_;
    std::vector<render_item> display_list_;

    float cursor_x_ = constants::h_step;
    float cursor_y_ = constants::v_step;
    int weight_ = sf::Text::Style::Regular;
    int style_ = sf::Text::Style::Regular;
    int size_ = constants::font_size;

    float x_{};
    float y_{};
    float height_{};

    // Stored as pointer to allow assignment/copying.
    const sf::Font* font_;

    static constexpr auto block_elements_ = std::to_array(
        {"html", "body",     "article", "section",    "nav",        "aside",  "h1",     "h2",
         "h3",   "h4",       "h5",      "h6",         "hgroup",     "header", "footer", "address",
         "p",    "hr",       "pre",     "blockquote", "ol",         "ul",     "menu",   "li",
         "dl",   "dt",       "dd",      "figure",     "figcaption", "main",   "div",    "table",
         "form", "fieldset", "legend",  "details",    "summary"}
    );
};
