#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

struct text_data {
    std::string text;
};

struct element_data {
    std::string tag;
    std::unordered_map<std::string, std::string> attributes;
};

struct html_node {
    std::variant<text_data, element_data> data;

    // @TODO: Text nodes don't have children but is here for simplificaiton. Maybe remove this
    // later.
    std::vector<std::unique_ptr<html_node>> children;

    // @Cleanup: Maybe move this to just element_data.
    std::unordered_map<std::string, std::string> style;

    html_node* parent = nullptr;
};

class html_parser {
public:
    explicit html_parser(std::string_view body) : body_{body} {}

    [[nodiscard]] std::unique_ptr<html_node> parse();

private:
    void add_text(std::string_view text);
    void add_tag(std::string_view raw_tag);

    [[nodiscard]] static std::pair<std::string, std::unordered_map<std::string, std::string>>
    get_attributes(std::string_view text);

    void implicit_tags(std::optional<std::string_view> tag = std::nullopt);

    [[nodiscard]] std::unique_ptr<html_node> finish();

    std::string_view body_;
    std::vector<std::unique_ptr<html_node>> unfinished_;

    static constexpr auto self_closing_tags_ = std::to_array(
        {"area",
         "base",
         "br",
         "col",
         "embed",
         "hr",
         "img",
         "input",
         "link",
         "meta",
         "param",
         "source",
         "track",
         "wbr"}
    );
    static constexpr auto head_tags_ = std::to_array({
        "base",
        "basefont",
        "bgsound",
        "noscript",
        "link",
        "meta",
        "title",
        "style",
        "script",
    });
};
