#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <browser/constants.hpp>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
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
    TTF_Text* text = nullptr;
};

struct LineItem {
    float x{};
    TTF_Text* text = nullptr;
    TTF_Font* font = nullptr;
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
                TTF_TextEngine* text_engine,
                FontCache* font_cache,
                float width)
        : node_{n},
          parent_{parent},
          previous_{previous},
          text_engine_{text_engine},
          font_cache_{font_cache},
          width_{width} {}

    void layout();
    void flush();

    [[nodiscard]] std::vector<DrawCmd> paint();

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

    FontCache* font_cache_;
    TTF_TextEngine* text_engine_;
};
