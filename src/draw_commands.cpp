#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <browser/draw_commands.hpp>


DrawRectCmd::DrawRectCmd(float left, float top, float right, float bottom, SDL_Color color) noexcept
    : top_{top}, left_{left}, bottom_{bottom}, right_{right}, color_{color} {}

void DrawRectCmd::execute(float scroll, SDL_Renderer* renderer) const noexcept {
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
    SDL_FRect rect{.x = left_, .y = top_ - scroll, .w = right_ - left_, .h = bottom_ - top_};
    SDL_RenderFillRect(renderer, &rect);
}

DrawTextCmd::DrawTextCmd(float left, float top, TTF_Text* text) noexcept
    : top_{top}, left_{left}, text_{text} {
    int width = 0;
    int height = 0;
    TTF_GetTextSize(text_, &width, &height);
    bottom_ = top_ + static_cast<float>(height);
}

void DrawTextCmd::execute(float scroll, SDL_Renderer*) const noexcept {
    TTF_DrawRendererText(text_, left_, top_ - scroll);
}
