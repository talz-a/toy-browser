#include <algorithm>
#include <array>
#include <browser/block_layout.hpp>
#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_node.hpp>
#include <browser/utils.hpp>
#include <memory>
#include <ranges>
#include <sstream>
#include <string_view>
#include <variant>

namespace {
constexpr float LINE_HEIGHT_MULTIPLIER = 1.25f;

constexpr std::array<std::string_view, 42> BLOCK_ELEMENTS = {
    "html", "body",     "article", "section",    "nav",        "aside",  "h1",     "h2",
    "h3",   "h4",       "h5",      "h6",         "hgroup",     "header", "footer", "address",
    "p",    "hr",       "pre",     "blockquote", "ol",         "ul",     "menu",   "li",
    "dl",   "dt",       "dd",      "figure",     "figcaption", "main",   "div",    "table",
    "form", "fieldset", "legend",  "details",    "summary"};
}  // namespace

BlockLayout::BlockLayout(const HTMLNode* node,
                         LayoutParent parent,
                         const BlockLayout* previous,
                         TTF_TextEngine* text_engine,
                         FontCache* font_cache,
                         float scale)
    : node_{node},
      parent_{parent},
      previous_{previous},
      scale_{scale},
      font_cache_{font_cache},
      text_engine_{text_engine} {}

BlockLayout::~BlockLayout() = default;

LineLayout::LineLayout(BlockLayout* parent, LineLayout* previous)
    : parent_{parent}, previous_{previous} {}

TextLayout::TextLayout(const HTMLNode* node,
                       TTF_Text* text,
                       TTF_Font* font,
                       LineLayout* parent,
                       TextLayout* previous)
    : node_{node}, text_{text}, font_{font}, parent_{parent}, previous_{previous} {}

TextLayout::~TextLayout() = default;

void LineLayout::layout() {
    width_ = parent_->width();
    x_ = parent_->x();

    if (previous_) {
        y_ = previous_->y_ + previous_->height_;
    } else {
        y_ = parent_->y();
    }

    if (children_.empty()) {
        height_ = 0.f;
        return;
    }

    for (auto& word : children_) {
        word->layout();
    }

    float max_ascent = 0.f;
    float max_descent = 0.f;

    for (const auto& word : children_) {
        float ascent = static_cast<float>(TTF_GetFontAscent(word->font_));
        float descent = std::abs(static_cast<float>(TTF_GetFontDescent(word->font_)));
        max_ascent = std::max(max_ascent, ascent);
        max_descent = std::max(max_descent, descent);
    }

    const float baseline = y_ + (LINE_HEIGHT_MULTIPLIER * max_ascent);

    for (auto& word : children_) {
        float ascent = static_cast<float>(TTF_GetFontAscent(word->font_));
        word->y_ = baseline - ascent;
    }

    height_ = LINE_HEIGHT_MULTIPLIER * (max_ascent + max_descent);
}

void TextLayout::layout() {
    int word_width = 0;
    int word_height = 0;
    TTF_GetTextSize(text_.get(), &word_width, &word_height);
    width_ = static_cast<float>(word_width);

    if (previous_) {
        int space_width = 0;
        int space_height = 0;
        TTF_GetStringSize(previous_->font_, " ", 0, &space_width, &space_height);
        x_ = previous_->x_ + static_cast<float>(space_width) + previous_->width_;
    } else {
        x_ = parent_->x();
    }

    float ascent = static_cast<float>(TTF_GetFontAscent(font_));
    float descent = std::abs(static_cast<float>(TTF_GetFontDescent(font_)));
    height_ = LINE_HEIGHT_MULTIPLIER * (ascent + descent);
}

std::vector<DrawCmd> TextLayout::paint() const {
    return {DrawTextCmd{x_, y_, text_.get()}};
}

void BlockLayout::new_line() {
    cursor_x_ = 0.f;
    LineLayout* last_line = lines_.empty() ? nullptr : lines_.back().get();
    lines_.push_back(std::make_unique<LineLayout>(this, last_line));
}

void BlockLayout::layout() {
    const auto [p_x, p_y, p_width] = std::visit(
        [](const auto* parent) { return std::tuple{parent->x(), parent->y(), parent->width()}; },
        parent_);

    x_ = p_x;
    width_ = p_width;
    y_ = previous_ ? (previous_->y() + previous_->height()) : p_y;

    children_.clear();
    lines_.clear();

    const auto mode = layout_mode();

    if (mode == LayoutMode::Block) {
        BlockLayout* previous = nullptr;

        for (const auto& child : node_->children) {
            // Create new child layout.
            auto next = std::make_unique<BlockLayout>(
                child.get(), this, previous, text_engine_, font_cache_, scale_);

            // Get a raw pointer to use as 'previous' for next layout.
            previous = next.get();

            // Move ownership into children_.
            children_.push_back(std::move(next));
        }

        // This needs to be after as the height of a layout depends on the layout of it's childern.
        for (const auto& child : children_) {
            child->layout();
        }

        height_ = std::ranges::fold_left(
            children_, 0.f, [](float sum, const auto& child) { return sum + child->height(); });
    } else {
        cursor_x_ = 0.f;
        new_line();
        recurse(*node_);

        for (auto& line : lines_) {
            line->layout();
        }

        height_ = std::ranges::fold_left(
            lines_, 0.f, [](float sum, const auto& line) { return sum + line->height_; });
    }
}

std::vector<DrawCmd> BlockLayout::paint() const {
    std::vector<DrawCmd> cmds;

    if (std::holds_alternative<Element>(node_->data)) {
        const auto background = node_->style.find("background-color");
        if (background == node_->style.end() || background->second == "transparent") return cmds;

        const float x2 = x_ + width_;
        const float y2 = y_ + height_;
        cmds.emplace_back(DrawRectCmd(x_, y_, x2, y2, parse_color(background->second)));
    }

    return cmds;
}

LayoutMode BlockLayout::layout_mode() const {
    return std::visit(
        match{
            [](const Text&) { return LayoutMode::InlineContext; },
            [&](const Element&) {
                const bool has_block_child =
                    std::ranges::any_of(node_->children, [](const auto& child) {
                        const auto* element = std::get_if<Element>(&child->data);
                        return element && std::ranges::contains(BLOCK_ELEMENTS, element->tag);
                    });

                if (has_block_child) return LayoutMode::Block;
                return node_->children.empty() ? LayoutMode::Block : LayoutMode::InlineContext;
            },
        },
        node_->data);
}

void BlockLayout::recurse(const HTMLNode& node) {
    std::visit(match{
                   [&](const Text& text) {
                       // @NOTE: We use stream extraction here to perfectly mimic Python's
                       // string.split(). The stream automatically normalizes the text by skipping
                       // over \n, \t, and collapsing consecutive whitespace.
                       std::istringstream words{text.text};

                       for (const auto& current_word : std::views::istream<std::string>(words)) {
                           word(node, current_word);
                       }
                   },
                   [&](const Element& element) {
                       if (element.tag == "br") new_line();

                       for (const auto& child : node.children) {
                           recurse(*child);
                       }
                   },
               },
               node.data);
}

void BlockLayout::word(const HTMLNode& node, const std::string& word_text) {
    std::string weight = node.style.at("font-weight");
    std::string style = node.style.at("font-style");
    if (style == "normal") style = "roman";

    float fs_px = std::stof(node.style.at("font-size"));
    int size_pt = static_cast<int>(fs_px * scale_ * 1.25f);

    TTF_Font* current_font = font_cache_->get_font(size_pt, weight, style);

    SDL_Color color = parse_color(node.style.at("color"));

    TTF_Text* word_sdl = TTF_CreateText(text_engine_, current_font, word_text.c_str(), 0);
    TTF_SetTextColor(word_sdl, color.r, color.g, color.b, color.a);

    int word_width = 0;
    int word_height = 0;
    TTF_GetTextSize(word_sdl, &word_width, &word_height);

    if (cursor_x_ + word_width > width_) new_line();

    LineLayout* line = lines_.back().get();
    TextLayout* previous_word = line->children_.empty() ? nullptr : line->children_.back().get();
    line->children_.push_back(
        std::make_unique<TextLayout>(&node, word_sdl, current_font, line, previous_word));

    int space_width = 0;
    int space_height = 0;
    TTF_GetStringSize(current_font, " ", 0, &space_width, &space_height);
    cursor_x_ += word_width + space_width;
}
