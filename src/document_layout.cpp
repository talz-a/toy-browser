#include "browser/document_layout.hpp"
#include "browser/block_layout.hpp"

void document_layout::layout() {
    auto child = std::make_unique<block_layout>(node_, this, nullptr, *font_, width_);

    child->layout();

    display_list_ = child->get_display_list();

    children_.push_back(std::move(child));
}
