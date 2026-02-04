#pragma once

#include "block_layout.hpp"
#include "html_parser.hpp"

struct document_layout {
    document_layout(const node* n, const sf::Font& font, float width)
        : node_{n}, font_{&font}, width_{width} {}

    void layout();

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }

    // For debug printing the tree.
    [[nodiscard]] const block_layout* get_root() const {
        return children_.empty() ? nullptr : children_.front().get();
    }

    const node* node_;
    const block_layout* parent_ = nullptr;
    std::vector<std::unique_ptr<block_layout>> children_;

    const sf::Font* font_;
    float width_;

    float height_{};
    float x_{};
    float y_{};

    std::vector<render_item> display_list_;
};
