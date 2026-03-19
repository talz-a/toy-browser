#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <browser/block_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
#include <browser/html_parser.hpp>

struct DocumentLayout {
    DocumentLayout(const HTMLNode* n,
                   TTF_TextEngine* text_engine,
                   FontCache* font_cache,
                   float width)
        : node_{n}, text_engine_{text_engine}, font_cache_{font_cache}, width_{width} {}

    void layout();

    [[nodiscard]] std::vector<DrawCmd> paint() const { return display_list_; };

    const HTMLNode* node_;
    const BlockLayout* parent_ = nullptr;
    float width_;

    std::vector<std::unique_ptr<BlockLayout>> children_;
    std::vector<DrawCmd> display_list_;

    float height_{};
    float x_{};
    float y_{};

    FontCache* font_cache_;
    TTF_TextEngine* text_engine_;
};
