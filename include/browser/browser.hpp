#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
#include <browser/html_parser.hpp>
#include <browser/url.hpp>
#include <expected>

struct Browser {
    Browser();
    ~Browser();

    std::expected<void, std::string> load(const Url& target_url);

    void run();
    void process_events();
    void render();

    float scroll_ = 0.0f;
    std::optional<DocumentLayout> document_;

    bool is_running_ = false;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    FontCache font_cache_{};
    TTF_TextEngine* text_engine_ = nullptr;

    std::vector<DrawCmd> display_list_;
    std::unique_ptr<HTMLNode> nodes_;
};
