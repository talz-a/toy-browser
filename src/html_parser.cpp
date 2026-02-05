#include "browser/html_parser.hpp"
#include <algorithm>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#include "browser/utils.hpp"

std::unique_ptr<html_node> html_parser::parse() {
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

void html_parser::add_text(std::string_view text) {
    if (text == " ") return;

    implicit_tags();

    if (unfinished_.empty()) return;

    const auto& parent = unfinished_.back();

    auto new_node = std::make_unique<html_node>();
    new_node->data = text_data{.text = std::string(text)};
    new_node->parent = parent.get();

    parent->children.push_back(std::move(new_node));
}

void html_parser::add_tag(std::string_view raw_tag) {
    auto [tag, attributes] = get_attributes(raw_tag);

    if (tag.starts_with('!')) return;

    implicit_tags(tag);

    if (tag.starts_with('/')) {
        if (unfinished_.size() == 1) return;

        auto node = std::move(unfinished_.back());
        unfinished_.pop_back();

        const auto& parent = unfinished_.back();
        parent->children.push_back(std::move(node));

    } else if (std::ranges::contains(self_closing_tags_, tag)) {
        const auto& parent = unfinished_.back();

        auto new_node = std::make_unique<html_node>();
        new_node->data = element_data{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent.get();

        parent->children.push_back(std::move(new_node));
    } else {
        html_node* parent = unfinished_.empty() ? nullptr : unfinished_.back().get();

        auto new_node = std::make_unique<html_node>();
        new_node->data = element_data{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent;

        unfinished_.push_back(std::move(new_node));
    }
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

void html_parser::implicit_tags(std::optional<std::string_view> tag) {
    while (true) {
        const auto open_tags = unfinished_ | std::views::filter([](const auto& node) {
                                   return std::holds_alternative<element_data>(node->data);
                               }) |
                               std::views::transform([](const auto& node) -> std::string_view {
                                   return std::get<element_data>(node->data).tag;
                               }) |
                               std::ranges::to<std::vector<std::string_view>>();

        // 1. If empty and first tag isn't <html>, add <html>.
        if (open_tags.empty() && tag != "html") {
            add_tag("html");
        }

        // 2. If we only have <html>, decide between <head> and <body>.
        else if (open_tags.size() == 1 && open_tags[0] == "html" && tag != "head" &&
                 tag != "body" && tag != "/html") {
            if (tag.has_value() && std::ranges::contains(head_tags_, *tag)) {
                add_tag("head");
            } else {
                add_tag("body");
            }
        }

        // 3. If in <head> and a body-tag arrives, close the <head>.
        else if (open_tags.size() == 2 && open_tags[0] == "html" && open_tags[1] == "head" &&
                 tag != "/head" && (tag.has_value() && !std::ranges::contains(head_tags_, *tag))) {
            add_tag("/head");
        }

        else {
            break;
        }
    }
}

std::unique_ptr<html_node> html_parser::finish() {
    if (unfinished_.empty()) return nullptr;

    implicit_tags();

    while (unfinished_.size() > 1) {
        auto node = std::move(unfinished_.back());
        unfinished_.pop_back();

        auto const& parent = unfinished_.back();
        parent->children.push_back(std::move(node));
    }

    auto root = std::move(unfinished_.back());
    unfinished_.pop_back();
    return root;
}
