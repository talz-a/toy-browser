#include <browser/draw_commands.hpp>

void DrawRect::execute(float scroll, SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
    SDL_FRect rect{.x = left_, .y = top_ - scroll, .w = right_ - left_, .h = bottom_ - top_};
    SDL_RenderFillRect(renderer, &rect);
}

void DrawText::execute(float scroll, SDL_Renderer* renderer) const {
    TTF_DrawRendererText(text_, left_, top_ - scroll);
}
