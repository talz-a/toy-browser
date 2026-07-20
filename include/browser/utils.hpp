#pragma once

#include <SDL3/SDL.h>
#include <algorithm>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

template <typename... Fs>
struct match : Fs... {
    using Fs::operator()...;
};

// @TODO: Maybe move this to a better place. Make sure to double check this works when using it with
// BlockLayout.
template <typename Tree>
void tree_to_list(Tree& tree, std::vector<Tree*>& list) {
    list.push_back(&tree);
    for (const auto& child : tree.children) {
        if (child) {
            tree_to_list(*child, list);
        }
    }
}

inline std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline bool is_space(unsigned char c) {
    return std::isspace(c);
};

inline std::string to_lower(std::string_view str) {
    return str | std::views::transform([](unsigned char c) { return std::tolower(c); }) |
           std::ranges::to<std::string>();
}

inline void to_lower_inplace(std::string& str) {
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
}

inline SDL_Color parse_color(const std::string& name) {
    static const std::unordered_map<std::string, SDL_Color> colors = {
        {"red", {255, 0, 0, 255}},
        {"green", {0, 255, 0, 255}},
        {"blue", {0, 0, 255, 255}},
        {"lightblue", {173, 216, 230, 255}},
        {"black", {0, 0, 0, 255}},
        {"gray", {192, 192, 192, 255}},
        {"#ddd", {221, 221, 221, 255}},
        {"white", {255, 255, 255, 255}},
        {"yellow", {255, 255, 0, 255}},
        {"magenta", {255, 0, 255, 255}},
        {"cyan", {0, 255, 255, 255}}};

    auto it = colors.find(name);
    if (it != colors.end()) {
        return it->second;
    }

    return {255, 255, 255, 255};
}
