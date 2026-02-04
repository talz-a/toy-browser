#include "browser/document_layout.hpp"
#include "browser/block_layout.hpp"
#include "browser/constants.hpp"

void document_layout::layout() {
    width_ = width_ - 2 * constants::h_step;
    x_ = constants::h_step;
    y_ = constants::v_step;

    auto child = std::make_unique<block_layout>(node_, this, nullptr, *font_, width_);
    child->layout();

    // Set height before moving...
    height_ = child->height_;

    display_list_ = child->get_display_list();
    children_.push_back(std::move(child));
}
