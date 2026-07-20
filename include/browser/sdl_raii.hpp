#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <format>
#include <memory>
#include <stdexcept>

struct SDLWindowDeleter {
    void operator()(SDL_Window* window) const noexcept { SDL_DestroyWindow(window); }
};

struct SDLRendererDeleter {
    void operator()(SDL_Renderer* renderer) const noexcept { SDL_DestroyRenderer(renderer); }
};

struct TTFTextEngineDeleter {
    void operator()(TTF_TextEngine* text_engine) const noexcept {
        TTF_DestroyRendererTextEngine(text_engine);
    }
};

struct TTFFontDeleter {
    void operator()(TTF_Font* font) const noexcept { TTF_CloseFont(font); }
};

struct TTFTextDeleter {
    void operator()(TTF_Text* text) const noexcept { TTF_DestroyText(text); }
};

using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
using SDLRendererPtr = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>;
using TTFTextEnginePtr = std::unique_ptr<TTF_TextEngine, TTFTextEngineDeleter>;
using TTFFontPtr = std::unique_ptr<TTF_Font, TTFFontDeleter>;
using TTFTextPtr = std::unique_ptr<TTF_Text, TTFTextDeleter>;

struct SDLRuntime {
    SDLRuntime() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error(std::format("Couldn't initialize SDL: {}", SDL_GetError()));
        }
    }

    SDLRuntime(const SDLRuntime&) = delete;
    SDLRuntime& operator=(const SDLRuntime&) = delete;

    ~SDLRuntime() { SDL_Quit(); }
};

struct TTFRuntime {
    TTFRuntime() {
        if (!TTF_Init()) {
            throw std::runtime_error(
                std::format("Couldn't initialize SDL_ttf: {}", SDL_GetError()));
        }
    }

    TTFRuntime(const TTFRuntime&) = delete;
    TTFRuntime& operator=(const TTFRuntime&) = delete;

    ~TTFRuntime() { TTF_Quit(); }
};