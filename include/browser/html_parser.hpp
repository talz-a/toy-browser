#pragma once

#include <browser/html_node.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class HTMLParser {
public:
    explicit HTMLParser(std::string_view body) : body_{body} {}

    [[nodiscard]] std::unique_ptr<HTMLNode> parse();

private:
    void add_text(std::string_view text);
    void add_tag(std::string_view raw_tag);

    [[nodiscard]] static std::pair<std::string, Attributes> parse_attributes(std::string_view text);

    void implicit_tags(std::optional<std::string_view> tag = std::nullopt);

    [[nodiscard]] std::unique_ptr<HTMLNode> finish();

    std::string_view body_;
    std::vector<std::unique_ptr<HTMLNode>> unfinished_;
};
