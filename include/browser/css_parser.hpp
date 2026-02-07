#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

class css_parser {
public:
    explicit css_parser(std::string_view s) : s_{s} {}

    void whitespace();
    std::string_view word();
    void literal(char ch);

    std::pair<std::string, std::string> pair();
    std::unordered_map<std::string, std::string> body();
    std::optional<char> ignore_until(const std::vector<char>& chars);

private:
    std::string_view s_;
    std::size_t i_ = 0;
};
