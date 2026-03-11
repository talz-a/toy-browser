#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace std::literals; 

constexpr std::array SELF_CLOSING_TAGS = {
    "area"sv,
    "base"sv,
    "br"sv,
    "col"sv,
    "embed"sv,
    "hr"sv,
    "img"sv,
    "input"sv,
    "link"sv,
    "meta"sv,
    "param"sv,
    "source"sv,
    "track"sv,
    "wbr"sv
};

constexpr std::array HEAD_TAGS = {
    "base"sv,
    "basefont"sv,
    "bgsound"sv,
    "noscript"sv,
    "link"sv,
    "meta"sv,
    "title"sv,
    "style"sv,
    "script"sv
};

using Attributes = std::unordered_map<std::string, std::string>;
using Tag = std::string;

struct Text {
    std::string text;
};

struct Element {
    Tag tag;
    Attributes attributes;
};

struct HTMLNode {
    std::variant<Text, Element> data;

    // @TODO: Text nodes don't have children but is here for simplificaiton. Maybe remove this later.
    std::vector<std::unique_ptr<HTMLNode>> children;

    // @TODO: Maybe move this to just element_data.
    Attributes style;

    HTMLNode* parent = nullptr;
};

struct HTMLParser {
    explicit HTMLParser(std::string_view body) : body_{body} {}

    [[nodiscard]] std::unique_ptr<HTMLNode> parse();

    void add_text(std::string_view text);
    void add_tag(std::string_view raw_tag);

    void implicit_tags(std::optional<std::string_view> tag = std::nullopt);

    [[nodiscard]] std::unique_ptr<HTMLNode> finish();

    [[nodiscard]] static std::pair<Tag, Attributes> parse_attributes(std::string_view text);

    std::string_view body_;
    std::vector<std::unique_ptr<HTMLNode>> unfinished_;
};
