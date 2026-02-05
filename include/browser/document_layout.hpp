#pragma once

#include "browser/block_layout.hpp"
#include "browser/draw_commands.hpp"
#include "html_parser.hpp"

struct document_layout {
    document_layout(const html_node* n, const sf::Font& font, float width)
        : node_{n}, font_{&font}, width_{width} {}

    void layout();

    [[nodiscard]] std::vector<draw_cmds> paint() const { return display_list_; };

    const html_node* node_;
    const block_layout* parent_ = nullptr;
    float width_;

    std::vector<std::unique_ptr<block_layout>> children_;
    std::vector<draw_cmds> display_list_;

    float height_{};
    float x_{};
    float y_{};

    // Stored as pointer to allow assignment/copying.
    const sf::Font* font_;
};
