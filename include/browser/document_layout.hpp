#pragma once

#include "block_layout.hpp"
#include "browser/draw_commands.hpp"
#include "html_parser.hpp"

struct document_layout {
    document_layout(const node* n, const sf::Font& font, float width)
        : node_{n}, font_{&font}, width_{width} {}

    void layout();

    [[nodiscard]] const std::vector<draw_cmds>& paint() const { return display_list_; };

    [[nodiscard]] const std::vector<std::unique_ptr<block_layout>>& get_children() const {
        return children_;
    }

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

    std::vector<draw_cmds> display_list_;
};
