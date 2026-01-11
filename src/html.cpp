#include "browser/html.hpp"

std::vector<token> lex(std::string_view body) {
    std::vector<token> out;
    std::string buffer;
    bool in_tag = false;

    for (char c : body) {
        if (c == '<') {
            in_tag = true;
            if (!buffer.empty()) out.emplace_back(text_token{.text = std::move(buffer)});
            buffer.clear();
        } else if (c == '>') {
            in_tag = false;
            out.emplace_back(tag_token{.tag = std::move(buffer)});
            buffer.clear();
        } else {
            // NOTE: Maybe handle the ending '/' of closing tags.
            if (c == '\n' || c == '\t') {
                buffer += ' ';
            } else {
                buffer += c;
            }
        }
    }

    if (!in_tag && !buffer.empty()) out.emplace_back(text_token{.text = std::move(buffer)});
    return out;
}
