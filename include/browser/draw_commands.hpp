#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <variant>

struct DrawRect {
    DrawRect(float x1, float y1, float x2, float y2, SDL_Color color)
        : top_{y1}, left_{x1}, bottom_{y2}, right_{x2}, color_{color} {}

    void execute(float scroll, SDL_Renderer* renderer) const;

    float top_;
    float left_;
    float bottom_;
    float right_;
    SDL_Color color_;
};

struct DrawText {
    DrawText(float x1, float y1, TTF_Text* text) : top_{y1}, left_{x1}, text_{text} {
        int width, height;
        TTF_GetTextSize(text_, &width, &height);
        bottom_ = y1 + static_cast<float>(height);
    }

    void execute(float scroll, SDL_Renderer* renderer) const;

    float top_;
    float left_;
    float bottom_;
    TTF_Text* text_;
};

using DrawCmd = std::variant<DrawRect, DrawText>;
