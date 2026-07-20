#pragma once

#include <SDL3/SDL_pixels.h>
#include <variant>

struct SDL_Renderer;
struct TTF_Text;

class DrawRectCmd {
public:
    DrawRectCmd(float left, float top, float right, float bottom, SDL_Color color) noexcept;

    void execute(float scroll, SDL_Renderer* renderer) const noexcept;

    [[nodiscard]] float top() const noexcept { return top_; }
    [[nodiscard]] float bottom() const noexcept { return bottom_; }

private:
    float top_;
    float left_;
    float bottom_;
    float right_;
    SDL_Color color_;
};

class DrawTextCmd {
public:
    DrawTextCmd(float left, float top, TTF_Text* text) noexcept;

    void execute(float scroll, SDL_Renderer* renderer) const noexcept;

    [[nodiscard]] float top() const noexcept { return top_; }
    [[nodiscard]] float bottom() const noexcept { return bottom_; }

private:
    float top_;
    float left_;
    float bottom_;
    TTF_Text* text_;
};

using DrawCmd = std::variant<DrawRectCmd, DrawTextCmd>;
