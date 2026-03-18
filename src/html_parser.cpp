#include <algorithm>
#include <browser/html_parser.hpp>
#include <browser/utils.hpp>
#include <ranges>
#include <string>
#include <vector>

std::unique_ptr<HTMLNode> HTMLParser::parse() {
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

void HTMLParser::add_text(std::string_view text) {
    if (std::ranges::all_of(text, is_space)) return;

    implicit_tags();

    if (unfinished_.empty()) return;

    const auto& parent = unfinished_.back();

    auto new_node = std::make_unique<HTMLNode>();
    new_node->data = Text{.text = std::string(text)};
    new_node->parent = parent.get();

    parent->children.push_back(std::move(new_node));
}

void HTMLParser::add_tag(std::string_view raw_tag) {
    auto [tag, attributes] = parse_attributes(raw_tag);

    if (tag.starts_with('!')) return;

    implicit_tags(tag);

    if (tag.starts_with('/')) {
        // Protect root node.
        if (unfinished_.size() == 1) return;

        auto node = std::move(unfinished_.back());
        unfinished_.pop_back();

        const auto& parent = unfinished_.back();
        parent->children.push_back(std::move(node));

    } else if (std::ranges::contains(SELF_CLOSING_TAGS, tag)) {
        const auto& parent = unfinished_.back();

        auto new_node = std::make_unique<HTMLNode>();
        new_node->data = Element{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent.get();

        parent->children.push_back(std::move(new_node));
    } else {
        HTMLNode* parent = unfinished_.empty() ? nullptr : unfinished_.back().get();

        auto new_node = std::make_unique<HTMLNode>();
        new_node->data = Element{.tag = std::string(tag), .attributes = std::move(attributes)};
        new_node->parent = parent;

        unfinished_.push_back(std::move(new_node));
    }
}

std::pair<Tag, Attributes> HTMLParser::parse_attributes(std::string_view text) {
    Attributes attributes;
    if (text.empty()) return {"", attributes};

    // 1. Extract the tag name.
    auto tag_end = std::ranges::find_if(text, is_space);
    std::string tag = to_lower(std::string_view(text.begin(), tag_end));

    text = std::string_view(tag_end, text.end());

    // 2. Parse the attributes.
    while (!text.empty()) {
        // Drop leading whitespace.
        text = std::string_view(std::ranges::find_if_not(text, is_space), text.end());
        if (text.empty()) break;

        // Find the end of the key.
        auto key_end =
            std::ranges::find_if(text, [](unsigned char c) { return std::isspace(c) || c == '='; });

        std::string key = to_lower(std::string_view(text.begin(), key_end));
        text = std::string_view(key_end, text.end());

        std::string value;

        // If there's an '=', parse the value.
        if (!text.empty() && text.front() == '=') {
            // Drop the '='.
            text.remove_prefix(1);

            if (!text.empty() && (text.front() == '"' || text.front() == '\'')) {
                const char quote = text.front();
                text.remove_prefix(1);  // Drop opening quote.

                auto val_end = std::ranges::find(text, quote);
                value = std::string(text.begin(), val_end);

                text = std::string_view(val_end, text.end());
                if (!text.empty()) text.remove_prefix(1);  // Drop closing quote.
            } else {
                // Unquoted value.
                auto val_end = std::ranges::find_if(text, is_space);
                value = std::string(text.begin(), val_end);
                text = std::string_view(val_end, text.end());
            }
        }

        if (!key.empty()) {
            attributes.emplace(std::move(key), std::move(value));
        }
    }

    return {std::move(tag), std::move(attributes)};
}

void HTMLParser::implicit_tags(std::optional<std::string_view> tag) {
    while (true) {
        const auto open_tags =
            unfinished_ | std::views::filter([](auto&& node) {
                return std::holds_alternative<Element>(node->data);
            }) |
            std::views::transform([](auto&& node) { return std::get<Element>(node->data).tag; }) |
            std::ranges::to<std::vector>();

        // 1. If empty and first tag isn't <html>, add <html>.
        if (open_tags.empty() && tag != "html") {
            add_tag("html");
        }

        // 2. If we only have <html>, decide between <head> and <body>.
        else if (open_tags.size() == 1 && open_tags[0] == "html" && tag != "head" &&
                 tag != "body" && tag != "/html") {
            if (tag.has_value() && std::ranges::contains(HEAD_TAGS, *tag)) {
                add_tag("head");
            } else {
                add_tag("body");
            }
        }

        // 3. If in <head> and a body-tag (or text node) arrives, close the <head>.
        else if (open_tags.size() == 2 && open_tags[0] == "html" && open_tags[1] == "head" &&
                 tag != "/head" && (!tag.has_value() || !std::ranges::contains(HEAD_TAGS, *tag))) {
            add_tag("/head");
        }

        else {
            break;
        }
    }
}

std::unique_ptr<HTMLNode> HTMLParser::finish() {
    if (unfinished_.empty()) implicit_tags();

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
