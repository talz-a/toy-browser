#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <array>
#include <browser/constants.hpp>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
#include <browser/html_parser.hpp>
#include <memory>
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

enum class LayoutMode : std::uint8_t {
    Block,
    InlineContext,
};

struct DocumentLayout;
struct BlockLayout;
struct LineLayout;
struct TextLayout;
using LayoutParent = std::variant<DocumentLayout*, BlockLayout*>;

struct BlockLayout {
    BlockLayout(const HTMLNode* n,
                LayoutParent parent,
                const BlockLayout* previous,
                TTF_TextEngine* text_engine,
                FontCache* font_cache,
                float width,
                float scale)
        : node_{n},
          parent_{parent},
          previous_{previous},
          text_engine_{text_engine},
          font_cache_{font_cache},
          width_{width},
          scale_{scale} {}

    ~BlockLayout();

    void layout();
    void new_line();

    [[nodiscard]] std::vector<DrawCmd> paint();

    [[nodiscard]] LayoutMode get_layout_mode() const;

    void recurse(const HTMLNode* node);

    void word(const HTMLNode& node, const std::string& word);

    const HTMLNode* node_;
    LayoutParent parent_;
    const BlockLayout* previous_ = nullptr;
    float width_;
    float scale_;

    std::vector<std::unique_ptr<BlockLayout>> children_;
    std::vector<std::unique_ptr<LineLayout>> lines_;

    float cursor_x_ = constants::h_step;

    float x_{};
    float y_{};
    float height_{};

    FontCache* font_cache_;
    TTF_TextEngine* text_engine_;
};

struct LineLayout {
    LineLayout(const HTMLNode* node, BlockLayout* parent, LineLayout* previous)
        : node_{node}, parent_{parent}, previous_{previous} {}

    void layout();

    [[nodiscard]] std::vector<DrawCmd> paint() const { return {}; }

    const HTMLNode* node_;
    BlockLayout* parent_;
    LineLayout* previous_;
    std::vector<std::unique_ptr<TextLayout>> children_;

    float x_{};
    float y_{};
    float width_{};
    float height_{};
};

struct TextLayout {
    TextLayout(const HTMLNode* node,
               TTF_Text* text,
               TTF_Font* font,
               LineLayout* parent,
               TextLayout* previous)
        : node_{node}, text_{text}, font_{font}, parent_{parent}, previous_{previous} {}

    void layout();

    [[nodiscard]] std::vector<DrawCmd> paint() const;

    const HTMLNode* node_;
    TTF_Text* text_;
    TTF_Font* font_;
    LineLayout* parent_;
    TextLayout* previous_;

    float x_{};
    float y_{};
    float width_{};
    float height_{};
};

inline BlockLayout::~BlockLayout() = default;
