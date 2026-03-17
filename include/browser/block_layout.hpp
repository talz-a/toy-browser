#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <browser/constants.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <string>
#include <vector>

struct RenderItem {
    float x{}, y{};
    sf::Text text;
};

struct LineItem {
    float x{};
    sf::Text text;
};

enum class LayoutMode : std::uint8_t {
    block,
    inline_context,
};

// @TODO: See if we can get around this forward decl stuff.
struct DocumentLayout;
struct BlockLayout;
using LayoutParent = std::variant<DocumentLayout*, BlockLayout*>;

struct BlockLayout {
    BlockLayout(const HTMLNode* n,
                LayoutParent parent,
                const BlockLayout* previous,
                const sf::Font& font,
                float width)
        : node_{n}, parent_{parent}, previous_{previous}, font_{&font}, width_{width} {}

    void layout();
    void flush();

    [[nodiscard]] std::vector<draw_cmds> paint();

    [[nodiscard]] float get_ascent(const sf::Font& font, unsigned int size) const;
    [[nodiscard]] float get_descent(const sf::Font& font, unsigned int size) const;

    [[nodiscard]] LayoutMode get_layout_mode() const;

    void recurse(const HTMLNode* node);

    void word(const HTMLNode& node, const std::string& word);

    const HTMLNode* node_;
    LayoutParent parent_;
    const BlockLayout* previous_ = nullptr;
    float width_;

    std::vector<std::unique_ptr<BlockLayout>> children_;
    std::vector<LineItem> line_;
    std::vector<RenderItem> display_list_;

    float cursor_x_ = constants::h_step;
    float cursor_y_ = constants::v_step;

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
         "form", "fieldset", "legend",  "details",    "summary"});
};
