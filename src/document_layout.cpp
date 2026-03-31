#include <browser/block_layout.hpp>
#include <browser/constants.hpp>
#include <browser/document_layout.hpp>

void DocumentLayout::layout() {
    const float h_step = constants::h_step * scale_;
    const float v_step = constants::v_step * scale_;

    width_ = width_ - 2 * h_step;
    x_ = h_step;
    y_ = v_step;

    auto child = std::make_unique<BlockLayout>(
        node_, this, nullptr, text_engine_, font_cache_, width_, scale_);
    child->layout();

    // Set height before moving.
    height_ = child->height_;

    children_.push_back(std::move(child));
}
