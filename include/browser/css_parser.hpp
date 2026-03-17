#pragma once

#include <browser/html_parser.hpp>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class TagSelector;
class DescendantSelector;

using Selector = std::variant<TagSelector, std::unique_ptr<DescendantSelector>>;
using CSSPair = std::pair<std::string, std::string>;

bool matches_any(const Selector& sel, const HTMLNode& node);

struct TagSelector {
    TagSelector(std::string_view tag) : tag_{tag} {}

    [[nodiscard]] bool matches(const HTMLNode& node) const;
    std::string tag_;
    uint16_t priority_ = 1;
};

struct DescendantSelector {
    DescendantSelector(Selector ancestor, TagSelector descendant);

    [[nodiscard]] bool matches(const HTMLNode& node) const;

    Selector ancestor_;
    TagSelector descendant_;
    uint16_t priority_ = 1;
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

inline uint16_t selector_priority(const Selector& sel) {
    return std::visit(
        [](const auto& arg) -> uint16_t {
            if constexpr (requires { arg->priority_; }) {
                return arg->priority_;
            } else {
                return arg.priority_;
            }
        },
        sel);
}
