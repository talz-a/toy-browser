#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <browser/constants.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <string>
#include <vector>

using namespace std::literals;

static constexpr std::array BLOCK_ELEMENTS = {
    "html"sv,       "body"sv,    "article"sv, "section"sv, "nav"sv,  "aside"sv,      "h1"sv,
    "h2"sv,         "h3"sv,      "h4"sv,      "h5"sv,      "h6"sv,   "hgroup"sv,     "header"sv,
    "footer"sv,     "address"sv, "p"sv,       "hr"sv,      "pre"sv,  "blockquote"sv, "ol"sv,
    "ul"sv,         "menu"sv,    "li"sv,      "dl"sv,      "dt"sv,   "dd"sv,         "figure"sv,
    "figcaption"sv, "main"sv,    "div"sv,     "table"sv,   "form"sv, "fieldset"sv,   "legend"sv,
    "details"sv,    "summary"sv};

struct RenderItem {
    float x{}, y{};
    sf::Text text;
};

struct LineItem {
    float x{};
    sf::Text text;
};

enum class LayoutMode : std::uint8_t {
    Block,
    InlineContext,
};

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

    [[nodiscard]] std::vector<DrawCmd> paint();

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
};
