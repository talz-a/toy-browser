#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <vector>

class BlockLayout;
class FontCache;
struct HTMLNode;

class DocumentLayout {
public:
    DocumentLayout(const HTMLNode* n,
                   TTF_TextEngine* text_engine,
                   FontCache* font_cache,
                   float width,
                   float scale);

    ~DocumentLayout();

    DocumentLayout(const DocumentLayout&) = delete;
    DocumentLayout& operator=(const DocumentLayout&) = delete;
    DocumentLayout(DocumentLayout&&) = delete;
    DocumentLayout& operator=(DocumentLayout&&) = delete;

    void layout();

    [[nodiscard]] float x() const noexcept { return x_; }
    [[nodiscard]] float y() const noexcept { return y_; }
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }
    [[nodiscard]] const std::vector<std::unique_ptr<BlockLayout>>& children() const noexcept {
        return children_;
    }

private:
    const HTMLNode* node_;
    float viewport_width_;
    float scale_;

    std::vector<std::unique_ptr<BlockLayout>> children_;

    float width_{};
    float height_{};
    float x_{};
    float y_{};

    FontCache* font_cache_;
    TTF_TextEngine* text_engine_;
};
