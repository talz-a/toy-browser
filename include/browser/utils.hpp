#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <ranges>
#include <string>

inline std::string to_lower(std::string_view str) {
    return str | std::views::transform([](unsigned char c) { return std::tolower(c); }) |
           std::ranges::to<std::string>();
}

inline void to_lower_inplace(std::string& str) {
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
}

inline sf::Color parse_color(const std::string& name) {
    static const std::unordered_map<std::string, sf::Color> colors = {
        {"red", sf::Color::Red},
        {"green", sf::Color::Green},
        {"blue", sf::Color::Blue},
        {"lightblue", sf::Color::Blue},
        {"black", sf::Color::Black},
        {"gray", sf::Color(192, 192, 192)},
        {"white", sf::Color::White},
        {"yellow", sf::Color::Yellow},
        {"magenta", sf::Color::Magenta},
        {"cyan", sf::Color::Cyan}
    };

    auto it = colors.find(name);
    if (it != colors.end()) {
        return it->second;
    }

    return sf::Color::White;
}
