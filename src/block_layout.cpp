#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <browser/block_layout.hpp>
#include <browser/constants.hpp>
#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <browser/utils.hpp>
#include <memory>
#include <ranges>
#include <variant>

// @HACK: No native way to get ascent of a word as of right now...
float BlockLayout::get_ascent(const sf::Font& font, unsigned int size) const {
    if (size == 0) return 0.f;
    const float top = font.getGlyph(U'\u00CA', size, false, 0).bounds.position.y;
    return -top;
}

// @HACK: No native way to get descent of a word as of right now...
float BlockLayout::get_descent(const sf::Font& font, unsigned int size) const {
    if (size == 0) return 0.f;
    const auto glyph = font.getGlyph('p', size, false);
    return glyph.bounds.size.y + glyph.bounds.position.y;
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
            auto next = std::make_unique<BlockLayout>(child.get(), this, previous, *font_, width_);

            // Get a raw pointer to use as 'previous' for next layout.
            previous = next.get();

            // Move ownership into children_.
            children_.push_back(std::move(next));
        }

        for (const auto& child : children_) {
            child->layout();
        }

        // This needs to be after as the height of a layout depends on the layout of it's childern.
        height_ = std::ranges::fold_left(
            children_, 0.f, [](float sum, const auto& child) { return sum + child->height_; });
    } else {
        cursor_x_ = 0.f;
        cursor_y_ = 0.f;

        height_ = cursor_y_;

        recurse(node_);
        flush();

        height_ = cursor_y_;
    }
}

void BlockLayout::flush() {
    if (line_.empty()) return;

    float max_ascent = 0.f;
    float max_descent = 0.f;

    for (const auto& [x, text] : line_) {
        const auto& font = text.getFont();
        unsigned int size = text.getCharacterSize();
        const float ascent = get_ascent(font, size);
        const float descent = get_descent(font, size);
        max_ascent = std::max(max_ascent, ascent);
        max_descent = std::max(max_descent, descent);
    }

    const float baseline = cursor_y_ + constants::line_height_multiplier * max_ascent;

    for (auto& [rel_x, text] : line_) {
        const auto& font = text.getFont();
        unsigned int size = text.getCharacterSize();

        const float ascent = get_ascent(font, size);
        const float y = y_ + baseline - ascent;
        const float x = x_ + rel_x;

        display_list_.emplace_back(RenderItem{.x = x, .y = y, .text = std::move(text)});
    }

    cursor_y_ = baseline + (constants::line_height_multiplier * max_descent);
    cursor_x_ = 0.f;
    line_.clear();
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
            cmds.emplace_back(DrawRect(x_, y_, x2, y2, parse_color(bgcolor)));
        }
    }

    if (get_layout_mode() == LayoutMode::InlineContext) {
        for (auto& [x, y, word] : display_list_) {
            cmds.emplace_back(DrawText(x, y, std::move(word)));
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
                if (arg.tag == "br") flush();

                for (const auto& child : node->children) {
                    recurse(child.get());
                }
            }
        },
        node->data);
}

void BlockLayout::word(const HTMLNode& node, const std::string& word_text) {
    // 1. Resolve Font Size (e.g., "16px" -> 16). yeah this might be wrong.
    float fs_px = std::stof(node.style.at("font-size"));

    // 2. Resolve Font Style & Weight.
    uint32_t sfml_style = sf::Text::Regular;
    if (node.style.at("font-weight") == "bold") sfml_style |= sf::Text::Bold;
    if (node.style.at("font-style") == "italic") sfml_style |= sf::Text::Italic;

    // 3. Parse color.
    sf::Color color = parse_color(node.style.at("color"));

    // 4. Create SFML Text.
    sf::Text word_sf(*font_,
                     sf::String::fromUtf8(word_text.begin(), word_text.end()),
                     static_cast<unsigned int>(fs_px));
    word_sf.setStyle(sfml_style);
    word_sf.setFillColor(color);

    const float word_width = word_sf.getGlobalBounds().size.x;
    if (cursor_x_ + word_width > width_) flush();

    line_.push_back({cursor_x_, std::move(word_sf)});

    bool is_bold = (sfml_style & sf::Text::Bold);
    float space_width = font_->getGlyph(U' ', static_cast<unsigned int>(fs_px), is_bold).advance;
    cursor_x_ += word_width + space_width;
}
