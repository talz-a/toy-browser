#pragma once

#include <array>
#include <browser/html_node.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

inline constexpr std::string_view DEFAULT_FONT_SIZE = "16px";
inline constexpr std::array<std::pair<std::string_view, std::string_view>, 4> INHERITED_PROPERTIES =
    {{{"font-size", DEFAULT_FONT_SIZE},
      {"font-style", "normal"},
      {"font-weight", "normal"},
      {"color", "black"}}};

class TagSelector {
public:
    explicit TagSelector(std::string tag) : tag_{std::move(tag)} {}

    [[nodiscard]] bool matches(const HTMLNode& node) const;
    [[nodiscard]] static constexpr std::uint16_t priority() noexcept { return 1; }

private:
    std::string tag_;
};

class DescendantSelector;

using Selector = std::variant<TagSelector, std::unique_ptr<DescendantSelector>>;
using CSSPair = std::pair<std::string, std::string>;

[[nodiscard]] bool matches_any(const Selector& selector, const HTMLNode& node);
[[nodiscard]] std::uint16_t selector_priority(const Selector& selector);

class DescendantSelector {
public:
    DescendantSelector(Selector ancestor, TagSelector descendant);

    [[nodiscard]] bool matches(const HTMLNode& node) const;
    [[nodiscard]] std::uint16_t priority() const noexcept { return priority_; }

private:
    Selector ancestor_;
    TagSelector descendant_;
    std::uint16_t priority_;
};

using CSSBody = std::unordered_map<std::string, std::string>;

struct CSSRule {
    Selector selector;
    CSSBody declarations;
};

class CSSParser {
public:
    explicit CSSParser(std::string_view s) : s_{s} {}

    [[nodiscard]] CSSBody parse_declarations();
    [[nodiscard]] std::vector<CSSRule> parse();

private:
    void whitespace();

    [[nodiscard]] std::optional<std::string_view> word();
    [[nodiscard]] bool literal(char ch);
    [[nodiscard]] std::optional<CSSPair> pair();
    [[nodiscard]] std::optional<char> ignore_until(std::string_view chars);
    [[nodiscard]] std::optional<Selector> selector_node();
    [[nodiscard]] std::optional<CSSRule> rule();

    std::string_view s_;
    std::size_t i_ = 0;
};
