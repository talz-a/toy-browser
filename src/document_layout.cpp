#include <browser/block_layout.hpp>
#include <browser/document_layout.hpp>

namespace {
constexpr float HORIZONTAL_MARGIN = 13.0f;
constexpr float VERTICAL_MARGIN = 18.0f;
}  // namespace

DocumentLayout::DocumentLayout(const HTMLNode* node,
                               TTF_TextEngine* text_engine,
                               FontCache* font_cache,
                               float width,
                               float scale)
    : node_{node},
      viewport_width_{width},
      scale_{scale},
      font_cache_{font_cache},
      text_engine_{text_engine} {}

DocumentLayout::~DocumentLayout() = default;

void DocumentLayout::layout() {
    const float horizontal_margin = HORIZONTAL_MARGIN * scale_;
    const float vertical_margin = VERTICAL_MARGIN * scale_;

    width_ = viewport_width_ - 2 * horizontal_margin;
    x_ = horizontal_margin;
    y_ = vertical_margin;

    children_.clear();

    auto child =
        std::make_unique<BlockLayout>(node_, this, nullptr, text_engine_, font_cache_, scale_);
    child->layout();

    height_ = child->height() + 2 * vertical_margin;

    children_.push_back(std::move(child));
}
