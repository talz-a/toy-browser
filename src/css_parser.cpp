#include "browser/css_parser.hpp"
#include <cctype>
#include <format>
#include <stdexcept>
#include <string_view>
#include "browser/utils.hpp"

void css_parser::whitespace() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) {
        i_++;
    }
}

std::string_view css_parser::word() {
    std::size_t start = i_;
    while (i_ < s_.size()) {
        if (std::isalnum(static_cast<unsigned char>(s_[i_])) ||
            std::string_view("#-.%").contains(s_[i_])) {
            i_++;
        } else {
            break;
        }
    }

    if (!(i_ > start)) {
        throw std::runtime_error(std::format("ERROR: Parsing expected word at index {}", i_));
    }

    return s_.substr(start, i_ - start);
}

void css_parser::literal(char ch) {
    if (!(i_ < s_.size() && s_[i_] == ch)) {
        throw std::runtime_error(std::format("ERROR: Parsing literal at index {}.", i_));
    }
    i_++;
}

std::pair<std::string, std::string> css_parser::pair() {
    std::string_view prop = word();
    whitespace();
    literal(':');
    whitespace();
    std::string_view val = word();
    return {to_lower(prop), std::string(val)};
}

std::optional<char> css_parser::ignore_until(const std::vector<char>& chars) {
    while (i_ < s_.size()) {
        if (std::ranges::contains(chars, s_[i_])) {
            return s_[i_];
        } else {
            i_++;
        }
    }
    return std::nullopt;
}

std::unordered_map<std::string, std::string> css_parser::body() {
    std::unordered_map<std::string, std::string> pairs;
    while (i_ < s_.size()) {
        try {
            auto [prop, val] = pair();
            pairs[prop] = val;
            whitespace();
            literal(';');
            whitespace();
        } catch (const std::exception& e) {
            auto why = ignore_until({';'});
            if (why == ';') {
                literal(';');
                whitespace();
            } else {
                break;
            }
        }
    }
    return pairs;
}
