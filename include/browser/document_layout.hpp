#pragma once

#include "block_layout.hpp"
#include "html_parser.hpp"

class document_layout {
public:
    document_layout(const node* n, const sf::Font& font, float width)
        : node_{n}, font_{&font}, width_{width} {}

    void layout();

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }

    // For debug printing the tree.
    [[nodiscard]] const block_layout* get_root() const {
        return children_.empty() ? nullptr : children_.front().get();
    }

private:
    const node* node_;
    const block_layout* parent_ = nullptr;
    std::vector<std::unique_ptr<block_layout>> children_;

    const sf::Font* font_;
    float width_;

    std::vector<render_item> display_list_;
};
