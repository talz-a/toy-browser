#include <browser/block_layout.hpp>
#include <browser/constants.hpp>
#include <browser/document_layout.hpp>

void DocumentLayout::layout() {
    width_ = width_ - 2 * constants::h_step;
    x_ = constants::h_step;
    y_ = constants::v_step;

    auto child = std::make_unique<BlockLayout>(node_, this, nullptr, *font_, width_);
    child->layout();

    // Set height before moving.
    height_ = child->height_;

    children_.push_back(std::move(child));
}
