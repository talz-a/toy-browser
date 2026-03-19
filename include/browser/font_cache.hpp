#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <print>
#include <string>
#include <tuple>

struct FontCache {
    FontCache() = default;

    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;

    ~FontCache() { clear(); }

    void clear() {
        for (auto& [key, font] : fonts_) {
            if (font) TTF_CloseFont(font);
        }
        fonts_.clear();
    }

    TTF_Font* get_font(int size, const std::string& weight, const std::string& style) {
        auto key = std::make_tuple(size, weight, style);

        if (fonts_.contains(key)) {
            return fonts_[key];
        }

        TTF_Font* new_font =
            TTF_OpenFont("assets/Inter-VariableFont.ttf", static_cast<float>(size));

        if (!new_font) {
            std::println(stderr, "Warning: Failed to load font size {}", size);
            return nullptr;
        }

        int sdl_style = TTF_STYLE_NORMAL;
        if (weight == "bold") sdl_style |= TTF_STYLE_BOLD;
        if (style == "italic") sdl_style |= TTF_STYLE_ITALIC;

        TTF_SetFontStyle(new_font, sdl_style);
        TTF_SetFontHinting(new_font, TTF_HINTING_LIGHT);

        fonts_[key] = new_font;
        return new_font;
    }

    std::map<std::tuple<int, std::string, std::string>, TTF_Font*> fonts_;
};
