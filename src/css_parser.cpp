#include "browser/css_parser.hpp"
#include <cctype>
#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>
#include "browser/utils.hpp"

bool matches_any(const selector& sel, const html_node& node) {
    return std::visit(
        [&node](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<descendant_selector>>) {
                return arg->matches(node);
            } else {
                return arg.matches(node);
            }
        },
        sel
    );
}

bool tag_selector::matches(const html_node& node) const {
    auto* el = std::get_if<element_data>(&node.data);
    return el && el->tag == tag_;
}

bool descendant_selector::matches(const html_node& node) const {
    if (!matches_any(descendant_, node)) return false;

    const html_node* curr = node.parent;
    while (curr) {
        if (matches_any(ancestor_, *curr)) return true;
        curr = curr->parent;
    }

    return false;
}

void css_parser::whitespace() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) {
        i_++;
    }
}

std::string_view css_parser::word() {
    std::size_t start = i_;
    while (i_ < s_.size()) {
        if (std::isalnum(static_cast<unsigned char>(s_[i_])) ||
            std::string_view("#-.%").contains(s_[i_])) {
            i_++;
        } else {
            break;
        }
    }

    if (!(i_ > start)) {
        throw std::runtime_error(std::format("ERROR: Parsing expected word at index {}", i_));
    }

    return s_.substr(start, i_ - start);
}

void css_parser::literal(char ch) {
    if (!(i_ < s_.size() && s_[i_] == ch)) {
        throw std::runtime_error(std::format("ERROR: Parsing literal at index {}.", i_));
    }
    i_++;
}

std::pair<std::string, std::string> css_parser::pair() {
    std::string_view prop = word();
    whitespace();
    literal(':');
    whitespace();
    std::string_view val = word();
    return {to_lower(prop), std::string(val)};
}

std::optional<char> css_parser::ignore_until(const std::vector<char>& chars) {
    while (i_ < s_.size()) {
        if (std::ranges::contains(chars, s_[i_])) {
            return s_[i_];
        } else {
            i_++;
        }
    }
    return std::nullopt;
}

css_body css_parser::body() {
    std::unordered_map<std::string, std::string> pairs;
    while (i_ < s_.size()) {
        try {
            auto [prop, val] = pair();
            pairs[prop] = val;
            whitespace();
            literal(';');
            whitespace();
        } catch (const std::exception& e) {
            auto why = ignore_until({'}'});
            if (why == '}') {
                literal('}');
                whitespace();
            } else {
                break;
            }
        }
    }
    return pairs;
}

selector css_parser::selector_node() {
    selector out = tag_selector(to_lower(word()));
    whitespace();

    while (i_ < s_.size() && s_[i_] != '{') {
        std::string_view tag = word();
        tag_selector descendant = tag_selector(to_lower(tag));
        out = std::make_unique<descendant_selector>(std::move(out), std::move(descendant));
        whitespace();
    }

    return out;
}

std::vector<css_rule> css_parser::parse() {
    std::vector<css_rule> rules;

    while (i_ < s_.size()) {
        whitespace();
        auto selector = selector_node();
        literal('{');
        whitespace();
        auto css_body = body();
        literal('}');
        rules.emplace_back(std::move(selector), std::move(css_body));
    }

    return rules;
}
