#include "browser/block_layout.hpp"
#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <memory>
#include <ranges>
#include <variant>
#include "browser/constants.hpp"
#include "browser/document_layout.hpp"  // IWYU pragma: keep
#include "browser/draw_commands.hpp"
#include "browser/html_parser.hpp"

// No native way to get ascent of a word as of right now...
float block_layout::get_ascent(const sf::Font& font, unsigned int size) {
    if (size == 0) return 0.f;
    const float top = font.getGlyph(U'\u00CA', size, false, 0).bounds.position.y;
    return -top;
}

// No native way to get descent of a word as of right now...
float block_layout::get_descent(const sf::Font& font, unsigned int size) {
    if (size == 0) return 0.f;
    const auto glyph = font.getGlyph('p', size, false);
    return glyph.bounds.size.y + glyph.bounds.position.y;
}

void block_layout::layout() {
    x_ = std::visit([](auto&& arg) { return arg->x_; }, parent_);
    width_ = std::visit([](auto&& arg) { return arg->width_; }, parent_);

    if (previous_) {
        y_ = previous_->y_ + previous_->height_;
    } else {
        y_ = std::visit([](auto&& arg) { return arg->y_; }, parent_);
    }

    auto mode = layout_mode();

    if (mode == "block") {
        block_layout* previous = nullptr;

        for (const auto& child : node_->children) {
            // Create new child layout.
            auto next = std::make_unique<block_layout>(child.get(), this, previous, *font_, width_);

            // Get a raw pointer to use as 'previous' for next layout.
            previous = next.get();

            // Move ownership into chilren_.
            children_.push_back(std::move(next));
        }

        for (const auto& child : children_) {
            child->layout();
        }

        // This needs to be after as the height of a layout depends on the layout of it's childern.
        height_ = std::ranges::fold_left(children_, 0.f, [](float sum, const auto& child) {
            return sum + child->height_;
        });
    } else {
        cursor_x_ = 0.f;
        cursor_y_ = 0.f;

        height_ = cursor_y_;

        weight_ = sf::Text::Style::Regular;
        style_ = sf::Text::Style::Regular;
        size_ = constants::font_size;

        line_.clear();
        recurse(node_);
        flush();

        height_ = cursor_y_;
    }
}

std::vector<draw_cmds> block_layout::paint() {
    std::vector<draw_cmds> cmds;

    if (auto* el = std::get_if<element_data>(&node_->data)) {
        if (el->tag == "pre") {
            float x2 = x_ + width_;
            float y2 = y_ + height_;
            cmds.emplace_back(draw_rect(x_, y_, x2, y2, sf::Color::Cyan));
        }
    }

    if (layout_mode() == "inline") {
        for (const auto& [x, y, word] : display_list_) {
            cmds.emplace_back(draw_text(x, y, word));
        }
    }

    return cmds;
}

std::string_view block_layout::layout_mode() const {
    return std::visit(
        [&](auto&& arg) -> std::string_view {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, text_data>) {
                return "inline";
            }

            else if constexpr (std::is_same_v<T, element_data>) {
                bool has_block_child = std::ranges::any_of(node_->children, [](const auto& child) {
                    if (auto* el = std::get_if<element_data>(&child->data)) {
                        return std::ranges::contains(block_elements_, el->tag);
                    }
                    return false;
                });

                if (has_block_child) return "block";
            }

            return node_->children.empty() ? "block" : "inline";
        },
        node_->data
    );
}

void block_layout::recurse(const node* node) {
    if (!node) return;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, text_data>) {
                // We should be spliting on \n's and \t's like python does, but since C++
                // split does not, we need to normalize it here.
                std::string text = arg.text;
                std::ranges::replace_if(text, [](unsigned char c) { return std::isspace(c); }, ' ');

                for (const auto w : std::views::split(text, ' ')) {
                    if (w.empty()) continue;
                    word(std::ranges::to<std::string>(w));
                }

            } else if constexpr (std::is_same_v<T, element_data>) {
                open_tag(arg);
                for (const auto& child : node->children) {
                    recurse(child.get());
                }
                close_tag(arg);
            }
        },
        node->data
    );
}

void block_layout::open_tag(const element_data& element) {
    if (element.tag == "i") {
        style_ = sf::Text::Style::Italic;
    } else if (element.tag == "b") {
        weight_ = sf::Text::Style::Bold;
    } else if (element.tag == "small") {
        size_ -= constants::font_size_small_diff;
    } else if (element.tag == "big") {
        size_ += constants::font_size_big_diff;
    } else if (element.tag == "br") {
        flush();
    }
}

void block_layout::close_tag(const element_data& element) {
    if (element.tag == "i") {
        style_ = sf::Text::Style::Regular;
    } else if (element.tag == "b") {
        weight_ = sf::Text::Style::Regular;
    } else if (element.tag == "small") {
        size_ += constants::font_size_small_diff;
    } else if (element.tag == "big") {
        size_ -= constants::font_size_big_diff;
    } else if (element.tag == "p") {
        flush();
        cursor_y_ += constants::v_step;
    }
}

void block_layout::word(const std::string& word_text) {
    sf::Text word_sf(*font_, sf::String::fromUtf8(word_text.begin(), word_text.end()), size_);
    word_sf.setStyle(style_ | weight_);
    const auto word_width = word_sf.getGlobalBounds().size.x;

    // Maybe cache so we are not calculating space_width more than once?
    const sf::Text space_sf(*font_, " ", size_);
    const float space_width = space_sf.getGlobalBounds().size.x;

    if (cursor_x_ + word_width > width_) flush();

    line_.push_back({cursor_x_, std::move(word_sf)});

    cursor_x_ += word_width + space_width;
}

void block_layout::flush() {
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

        display_list_.emplace_back(render_item{.x = x, .y = y, .text = std::move(text)});
    }

    cursor_y_ = baseline + (constants::line_height_multiplier * max_descent);
    cursor_x_ = 0.f;
    line_.clear();
}
