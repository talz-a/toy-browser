#pragma once

#include <browser/sdl_raii.hpp>
#include <iostream>
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <utility>

class FontCache {
public:
    FontCache() = default;

    FontCache(const FontCache&) = delete;
    FontCache& operator=(const FontCache&) = delete;

    [[nodiscard]] TTF_Font* get_font(int point_size,
                                     std::string_view weight,
                                     std::string_view style) {
        FontKey key{point_size, std::string{weight}, std::string{style}};

        if (auto it = fonts_.find(key); it != fonts_.end()) {
            return it->second.get();
        }

        auto font = TTFFontPtr{TTF_OpenFont("assets/segoeui.ttf", static_cast<float>(point_size))};

        if (!font) {
            std::println(std::cerr, "Warning: Failed to load font size {}", point_size);
            return nullptr;
        }

        auto sdl_style = TTF_STYLE_NORMAL;
        if (weight == "bold") sdl_style |= TTF_STYLE_BOLD;
        if (style == "italic") sdl_style |= TTF_STYLE_ITALIC;

        TTF_SetFontStyle(font.get(), sdl_style);
        TTF_SetFontHinting(font.get(), TTF_HINTING_LIGHT);

        return fonts_.try_emplace(std::move(key), std::move(font)).first->second.get();
    }

private:
    struct FontKey {
        int point_size;
        std::string weight;
        std::string style;

        auto operator<=>(const FontKey&) const = default;
    };

    std::map<FontKey, TTFFontPtr> fonts_;
};
