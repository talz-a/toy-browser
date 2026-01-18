#include "browser/html_parser.hpp"
#include <algorithm>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#include "browser/utils.hpp"

std::shared_ptr<node> html_parser::parse() {
    std::string text;
    bool in_tag = false;

    for (char c : body_) {
        if (c == '<') {
            in_tag = true;
            if (!text.empty()) add_text(text);
            text.clear();
        } else if (c == '>') {
            in_tag = false;
            add_tag(text);
            text.clear();
        } else {
            text += c;
        }
    }

    if (!in_tag && !text.empty()) add_text(text);
    return finish();
}

std::pair<std::string, std::unordered_map<std::string, std::string>> html_parser::get_attributes(
    std::string_view text
) {
    const auto parts = text | std::views::split(' ') | std::ranges::to<std::vector<std::string>>();

    if (parts.empty()) return {"", {}};

    const auto tag = to_lower(parts[0]);

    std::unordered_map<std::string, std::string> attributes;
    for (const auto& attrpair : parts | std::views::drop(1)) {
        if (attrpair.contains('=')) {
            const auto pos = attrpair.find('=');
            const auto key = attrpair.substr(0, pos);
            auto value = attrpair.substr(pos + 1);

            if (value.size() > 2) {
                char first = value.front();
                char last = value.back();

                if ((first == '"' || first == '\'') && first == last) {
                    value = value.substr(1, value.size() - 2);
                }
            }

            attributes.emplace(to_lower(key), value);
        } else {
            attributes.emplace(to_lower(attrpair), "");
        }
    }

    return {tag, attributes};
}

void html_parser::add_text(std::string_view text) {
    if (text == " ") return;

    if (unfinished_.empty()) return;

    const auto& parent = unfinished_.back();

    auto new_node = std::make_shared<node>();
    new_node->data = text_data{.text = std::string(text)};
    new_node->parent = parent;

    parent->children.push_back(new_node);
}

void html_parser::add_tag(std::string_view raw_tag) {
    if (raw_tag.starts_with('!')) return;

    auto [tag, attributes] = get_attributes(raw_tag);

    if (tag.starts_with('/')) {
        if (unfinished_.size() == 1) return;

        const auto node = unfinished_.back();
        unfinished_.pop_back();

        const auto& parent = unfinished_.back();
        parent->children.push_back(node);

    } else if (std::ranges::contains(self_closing_tags_, tag)) {
        const auto& parent = unfinished_.back();

        auto new_node = std::make_shared<node>();
        new_node->data = element_data{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent;

        parent->children.push_back(new_node);
    } else {
        const auto parent = unfinished_.empty() ? nullptr : unfinished_.back();

        auto new_node = std::make_shared<node>();
        new_node->data = element_data{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent;

        unfinished_.push_back(new_node);
    }
}

std::shared_ptr<node> html_parser::finish() {
    if (unfinished_.empty()) return nullptr;

    while (unfinished_.size() > 1) {
        const auto node = unfinished_.back();
        unfinished_.pop_back();

        auto const& parent = unfinished_.back();
        parent->children.push_back(node);
    }

    const auto root = unfinished_.back();
    unfinished_.pop_back();
    return root;
}
