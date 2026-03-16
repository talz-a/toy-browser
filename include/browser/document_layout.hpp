#pragma once

#include <browser/block_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>

struct DocumentLayout {
    DocumentLayout(const HTMLNode* n, const sf::Font& font, float width)
        : node_{n}, font_{&font}, width_{width} {}

    void layout();

    // NOTE: why is this here
    [[nodiscard]] std::vector<draw_cmds> paint() const { return display_list_; };

    const HTMLNode* node_;
    const BlockLayout* parent_ = nullptr;
    float width_;

    std::vector<std::unique_ptr<BlockLayout>> children_;
    std::vector<draw_cmds> display_list_;

    float height_{};
    float x_{};
    float y_{};

    // Stored as pointer to allow assignment/copying.
    const sf::Font* font_;
};
