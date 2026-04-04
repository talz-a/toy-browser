#include <algorithm>
#include <browser/block_layout.hpp>
#include <browser/constants.hpp>
#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <browser/utils.hpp>
#include <memory>
#include <ranges>
#include <sstream>
#include <variant>

void LineLayout::layout() {
    width_ = parent_->width_;
    x_ = parent_->x_;

    if (previous_) {
        y_ = previous_->y_ + previous_->height_;
    } else {
        y_ = parent_->y_;
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

    const float baseline = y_ + (constants::line_height_multiplier * max_ascent);

    for (auto& word : children_) {
        float ascent = static_cast<float>(TTF_GetFontAscent(word->font_));
        word->y_ = baseline - ascent;
    }

    height_ = constants::line_height_multiplier * (max_ascent + max_descent);
}

void TextLayout::layout() {
    int word_width = 0;
    int word_height = 0;
    TTF_GetTextSize(text_, &word_width, &word_height);
    width_ = static_cast<float>(word_width);

    if (previous_) {
        int space_width = 0;
        int space_height = 0;
        TTF_GetStringSize(previous_->font_, " ", 0, &space_width, &space_height);
        x_ = previous_->x_ + static_cast<float>(space_width) + previous_->width_;
    } else {
        x_ = parent_->x_;
    }

    float ascent = static_cast<float>(TTF_GetFontAscent(font_));
    float descent = std::abs(static_cast<float>(TTF_GetFontDescent(font_)));
    height_ = constants::line_height_multiplier * (ascent + descent);
}

std::vector<DrawCmd> TextLayout::paint() const {
    return {DrawTextCmd{x_, y_, text_}};
}

void BlockLayout::new_line() {
    cursor_x_ = 0.f;
    LineLayout* last_line = lines_.empty() ? nullptr : lines_.back().get();
    lines_.push_back(std::make_unique<LineLayout>(node_, this, last_line));
}

void BlockLayout::layout() {
    const auto [p_x, p_y, p_width] =
        std::visit([](auto&& p) { return std::tuple{p->x_, p->y_, p->width_}; }, parent_);

    x_ = p_x;
    width_ = p_width;
    y_ = previous_ ? (previous_->y_ + previous_->height_) : p_y;

    auto mode = get_layout_mode();

    if (mode == LayoutMode::Block) {
        BlockLayout* previous = nullptr;

        for (const auto& child : node_->children) {
            // Create new child layout.
            auto next = std::make_unique<BlockLayout>(
                child.get(), this, previous, text_engine_, font_cache_, width_, scale_);

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
            children_, 0.f, [](float sum, const auto& child) { return sum + child->height_; });
    } else {
        lines_.clear();
        cursor_x_ = 0.f;
        new_line();
        recurse(node_);

        for (auto& line : lines_) {
            line->layout();
        }

        height_ = std::ranges::fold_left(
            lines_, 0.f, [](float sum, const auto& line) { return sum + line->height_; });
    }
}

std::vector<DrawCmd> BlockLayout::paint() {
    std::vector<DrawCmd> cmds;

    if (auto* el = std::get_if<Element>(&node_->data)) {
        std::string bgcolor = "transparent";
        if (node_->style.contains("background-color")) {
            bgcolor = node_->style.at("background-color");
        }

        if (bgcolor != "transparent") {
            float x2 = x_ + width_;
            float y2 = y_ + height_;
            cmds.emplace_back(DrawRectCmd(x_, y_, x2, y2, parse_color(bgcolor)));
        }
    }

    return cmds;
}

LayoutMode BlockLayout::get_layout_mode() const {
    return std::visit(
        [&]<typename T>(const T& arg) -> LayoutMode {
            if constexpr (std::is_same_v<T, Text>) {
                return LayoutMode::InlineContext;
            } else if constexpr (std::is_same_v<T, Element>) {
                bool has_block_child = std::ranges::any_of(node_->children, [&](const auto& child) {
                    auto* el = std::get_if<Element>(&child->data);
                    return el && std::ranges::contains(BLOCK_ELEMENTS, el->tag);
                });

                if (has_block_child) return LayoutMode::Block;
            }

            return node_->children.empty() ? LayoutMode::Block : LayoutMode::InlineContext;
        },
        node_->data);
}

void BlockLayout::recurse(const HTMLNode* node) {
    if (!node) return;

    std::visit(
        [&]<typename T>(const T& arg) {
            if constexpr (std::is_same_v<T, Text>) {
                // @NOTE: We use stream extraction here to perfectly mimic Python's string.split().
                // The stream automatically normalizes the text by skipping over \n, \t,
                // and collapsing consecutive whitespace.
                std::istringstream iss(arg.text);

                for (auto w : std::views::istream<std::string>(iss)) {
                    word(*node, std::move(w));
                }

            } else if constexpr (std::is_same_v<T, Element>) {
                if (arg.tag == "br") new_line();

                for (const auto& child : node->children) {
                    recurse(child.get());
                }
            }
        },
        node->data);
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
    TextLayout* previous_word =
        line->children_.empty() ? nullptr : line->children_.back().get();
    line->children_.push_back(
        std::make_unique<TextLayout>(&node, word_sdl, current_font, line, previous_word));

    int space_width = 0;
    int space_height = 0;
    TTF_GetStringSize(current_font, " ", 0, &space_width, &space_height);
    cursor_x_ += word_width + space_width;
}
