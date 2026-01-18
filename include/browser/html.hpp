#pragma once

#include <array>
#include <memory>
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

struct node {
    std::variant<text_data, element_data> data;

    // Text nodes don't have children but is here for simplificaiton.
    std::vector<std::shared_ptr<node>> children;

    std::weak_ptr<node> parent;
};

class html_parser {
public:
    explicit html_parser(std::string_view body) : body_{body} {}

    [[nodiscard]] std::shared_ptr<node> parse();

private:
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
    std::string_view body_;
    std::vector<std::shared_ptr<node>> unfinished_;

    [[nodiscard]] static std::pair<std::string, std::unordered_map<std::string, std::string>>
    get_attributes(std::string_view text);

    void add_text(std::string_view text);
    void add_tag(std::string_view raw_tag);

    [[nodiscard]] std::shared_ptr<node> finish();
};
