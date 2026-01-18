#pragma once

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
