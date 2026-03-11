#pragma once

#include <browser/html_parser.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <expected>

class TagSelector;
class DescendantSelector;

using Selector = std::variant<TagSelector, std::unique_ptr<DescendantSelector>>;
using CSSPair = std::pair<std::string, std::string>;

bool matches_any(const Selector& sel, const HTMLNode& node);

struct TagSelector {
    std::string tag_;

    [[nodiscard]] bool matches(const HTMLNode& node) const;
};

struct DescendantSelector {
    Selector ancestor_;
    TagSelector descendant_;

    [[nodiscard]] bool matches(const HTMLNode& node) const;
};

using CSSBody = std::unordered_map<std::string, std::string>;
using CSSRule = std::pair<Selector, CSSBody>;

struct CSSParser {
    void whitespace();

    std::expected<std::string_view, std::string> word();

    std::expected<void, std::string> literal(char ch);

    std::expected<CSSPair, std::string> pair();

    std::optional<char> ignore_until(const std::vector<char>& chars);

    CSSBody body();

    std::expected<Selector, std::string> selector_node();

    std::vector<CSSRule> parse();

    std::string_view s_;
    std::size_t i_ = 0;
};
