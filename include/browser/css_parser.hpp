#pragma once

#include <browser/html_parser.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class tag_selector;
class descendant_selector;

using selector = std::variant<tag_selector, std::unique_ptr<descendant_selector>>;

class tag_selector {
public:
    explicit tag_selector(std::string_view tag) : tag_(tag) {}

    [[nodiscard]] bool matches(const html_node& node) const;

private:
    std::string tag_;
};

class descendant_selector {
public:
    descendant_selector(selector ancestor, tag_selector descendant)
        : ancestor_(std::move(ancestor)), descendant_(std::move(descendant)) {}

    [[nodiscard]] bool matches(const html_node& node) const;

private:
    selector ancestor_;
    tag_selector descendant_;
};

using css_body = std::unordered_map<std::string, std::string>;
using css_rule = std::pair<selector, css_body>;

class css_parser {
public:
    explicit css_parser(std::string_view s) : s_{s} {}

    void whitespace();
    std::string_view word();
    void literal(char ch);
    selector selector_node();
    std::vector<css_rule> parse();

    std::pair<std::string, std::string> pair();
    css_body body();
    std::optional<char> ignore_until(const std::vector<char>& chars);

private:
    std::string_view s_;
    std::size_t i_ = 0;
};
