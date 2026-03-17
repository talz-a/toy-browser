#include <browser/css_parser.hpp>
#include <browser/utils.hpp>
#include <cctype>
#include <expected>
#include <format>
#include <memory>
#include <string_view>

bool matches_any(const Selector& sel, const HTMLNode& node) {
    return std::visit(
        [&node]<typename T>(const T& arg) -> bool {
            if constexpr (std::is_same_v<T, std::unique_ptr<DescendantSelector>>) {
                return arg->matches(node);
            } else {
                return arg.matches(node);
            }
        },
        sel);
}

bool TagSelector::matches(const HTMLNode& node) const {
    auto* el = std::get_if<Element>(&node.data);
    return el && el->tag == tag_;
}

DescendantSelector::DescendantSelector(Selector ancestor, TagSelector descendant)
    : ancestor_{std::move(ancestor)}, descendant_{std::move(descendant)} {
    priority_ = selector_priority(ancestor_) + descendant_.priority_;
}

bool DescendantSelector::matches(const HTMLNode& node) const {
    if (!matches_any(descendant_, node)) return false;

    const HTMLNode* curr = node.parent;
    while (curr) {
        if (matches_any(ancestor_, *curr)) return true;
        curr = curr->parent;
    }

    return false;
}

void CSSParser::whitespace() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) {
        i_++;
    }
}

std::expected<std::string_view, std::string> CSSParser::word() {
    size_t start = i_;
    while (i_ < s_.size()) {
        if (std::isalnum(static_cast<unsigned char>(s_[i_])) ||
            std::string_view("#-.%").contains(s_[i_])) {
            i_++;
        } else {
            break;
        }
    }

    if (!(i_ > start)) {
        return std::unexpected(std::format("ERROR: Parsing expected word at index {}", i_));
    }

    return s_.substr(start, i_ - start);
}

std::expected<void, std::string> CSSParser::literal(char ch) {
    if (!(i_ < s_.size() && s_[i_] == ch)) {
        return std::unexpected(std::format("ERROR: Parsing literal '{}' at index {}.", ch, i_));
    }
    i_++;
    return {};
}

std::expected<CSSPair, std::string> CSSParser::pair() {
    auto prop = word();
    if (!prop) return std::unexpected(prop.error());

    whitespace();

    auto lit = literal(':');
    if (!lit) return std::unexpected(lit.error());

    whitespace();

    auto val = word();
    if (!val) return std::unexpected(val.error());

    return CSSPair{to_lower(*prop), std::string(*val)};
}

std::optional<char> CSSParser::ignore_until(const std::vector<char>& chars) {
    while (i_ < s_.size()) {
        if (std::ranges::contains(chars, s_[i_])) {
            return s_[i_];
        } else {
            i_++;
        }
    }

    return std::nullopt;
}

CSSBody CSSParser::body() {
    CSSBody pairs;

    while (i_ < s_.size() && s_[i_] != '}') {
        auto parse_pair_sequence = [&]() -> std::expected<void, std::string> {
            auto parsed_pair = pair();
            if (!parsed_pair) return std::unexpected(parsed_pair.error());

            pairs[parsed_pair->first] = parsed_pair->second;
            whitespace();

            auto semi = literal(';');
            if (!semi) return std::unexpected(semi.error());

            whitespace();
            return {};
        }();

        if (!parse_pair_sequence) {
            auto why = ignore_until({';', '}'});
            if (why == ';') {
                // @NOTE: Fix this later.
                std::ignore = literal(';');
                whitespace();
            } else {
                break;
            }
        }
    }

    return pairs;
}

std::expected<Selector, std::string> CSSParser::selector_node() {
    auto first_tag = word();
    if (!first_tag) return std::unexpected(first_tag.error());

    Selector out = TagSelector(to_lower(*first_tag));
    whitespace();

    while (i_ < s_.size() && s_[i_] != '{') {
        auto tag = word();
        if (!tag) return std::unexpected(tag.error());

        TagSelector descendant = TagSelector(to_lower(*tag));
        out = std::make_unique<DescendantSelector>(std::move(out), std::move(descendant));
        whitespace();
    }

    return out;
}

std::vector<CSSRule> CSSParser::parse() {
    std::vector<CSSRule> rules;

    while (i_ < s_.size()) {
        auto parse_rule = [&]() -> std::expected<CSSRule, std::string> {
            whitespace();
            if (i_ >= s_.size()) return std::unexpected("EOF");

            auto selector = selector_node();
            if (!selector) return std::unexpected(selector.error());

            auto brace = literal('{');
            if (!brace) return std::unexpected(brace.error());

            whitespace();
            auto css_body = body();

            auto end_brace = literal('}');
            if (!end_brace) return std::unexpected(end_brace.error());

            return CSSRule{std::move(*selector), std::move(css_body)};
        }();

        if (parse_rule) {
            rules.push_back(std::move(*parse_rule));
        } else {
            auto why = ignore_until({'}'});
            if (why == '}') {
                // @NOTE: Fix this later.
                std::ignore = literal('}');
                whitespace();
            } else {
                break;
            }
        }
    }

    return rules;
}
