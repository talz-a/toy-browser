#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <map>
#include <string>
#include <tuple>

class FontCache {
public:
    // Destructor: Cleans up all cached fonts when the browser closes
    ~FontCache() {
        for (auto& [key, font] : fonts_) {
            if (font) TTF_CloseFont(font);
        }
        fonts_.clear();
    }

    // The C++ equivalent of your Python get_font function
    TTF_Font* get_font(int size, const std::string& weight, const std::string& style) {
        // Create our lookup key
        auto key = std::make_tuple(size, weight, style);

        // If it's already in the cache, return it immediately!
        if (fonts_.contains(key)) {
            return fonts_[key];
        }

        // If not found, we need to load a new instance of the font at this specific size
        // @NOTE: Update this path if your font is named differently!
        TTF_Font* new_font =
            TTF_OpenFont("assets/Inter-VariableFont.ttf", static_cast<float>(size));

        if (!new_font) {
            std::cerr << "Warning: Failed to load font size " << size << "\n";
            return nullptr;
        }

        // Apply SDL styles based on the CSS properties
        int sdl_style = TTF_STYLE_NORMAL;
        if (weight == "bold") sdl_style |= TTF_STYLE_BOLD;
        if (style == "italic") sdl_style |= TTF_STYLE_ITALIC;

        TTF_SetFontStyle(new_font, sdl_style);

        // Store it in the cache (the C++ equivalent of FONTS[key] = font)
        fonts_[key] = new_font;

        return new_font;
    }

private:
    // This std::map is the exact equivalent of your Python FONTS dictionary
    std::map<std::tuple<int, std::string, std::string>, TTF_Font*> fonts_;
};
