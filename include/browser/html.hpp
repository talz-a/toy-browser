#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct text_token {
    std::string text;
};

struct tag_token {
    std::string tag;
};

using token = std::variant<text_token, tag_token>;

[[nodiscard]] std::vector<token> lex(std::string_view body);
