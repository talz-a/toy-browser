#include <browser/css_parser.hpp>
#include <browser/utils.hpp>
#include <cctype>
#include <memory>
#include <string_view>

bool TagSelector::matches(const HTMLNode& node) const {
    const auto* element = std::get_if<Element>(&node.data);
    return element && element->tag == tag_;
}

DescendantSelector::DescendantSelector(Selector ancestor, TagSelector descendant)
    : ancestor_{std::move(ancestor)},
      descendant_{std::move(descendant)},
      priority_{static_cast<std::uint16_t>(selector_priority(ancestor_) + descendant_.priority())} {
}

bool DescendantSelector::matches(const HTMLNode& node) const {
    if (!descendant_.matches(node)) return false;

    for (const HTMLNode* curr = node.parent; curr != nullptr; curr = curr->parent) {
        if (matches_any(ancestor_, *curr)) return true;
    }

    return false;
}

bool matches_any(const Selector& selector, const HTMLNode& node) {
    return std::visit(match{
                          [&](const TagSelector& tag) { return tag.matches(node); },
                          [&](const std::unique_ptr<DescendantSelector>& descendant) {
                              return descendant->matches(node);
                          },
                      },
                      selector);
}

std::uint16_t selector_priority(const Selector& selector) {
    return std::visit(match{
                          [](const TagSelector& tag) { return tag.priority(); },
                          [](const std::unique_ptr<DescendantSelector>& descendant) {
                              return descendant->priority();
                          },
                      },
                      selector);
}

void CSSParser::whitespace() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) {
        i_++;
    }
}

std::optional<std::string_view> CSSParser::word() {
    const std::size_t start = i_;
    while (i_ < s_.size()) {
        if (std::isalnum(static_cast<unsigned char>(s_[i_])) ||
            std::string_view("#-.%").contains(s_[i_])) {
            i_++;
        } else {
            break;
        }
    }

    if (i_ == start) return std::nullopt;

    return s_.substr(start, i_ - start);
}

bool CSSParser::literal(char ch) {
    if (i_ >= s_.size() || s_[i_] != ch) return false;

    i_++;
    return true;
}

std::optional<CSSPair> CSSParser::pair() {
    const auto property = word();
    if (!property) return std::nullopt;

    whitespace();

    if (!literal(':')) return std::nullopt;

    whitespace();

    const auto value = word();
    if (!value) return std::nullopt;

    return CSSPair{to_lower(*property), std::string{*value}};
}

std::optional<char> CSSParser::ignore_until(std::string_view chars) {
    while (i_ < s_.size()) {
        if (chars.contains(s_[i_])) {
            return s_[i_];
        }
        i_++;
    }

    return std::nullopt;
}

CSSBody CSSParser::parse_declarations() {
    CSSBody declarations;

    while (i_ < s_.size() && s_[i_] != '}') {
        auto declaration = pair();
        if (declaration) {
            declarations.insert_or_assign(std::move(declaration->first),
                                          std::move(declaration->second));
            whitespace();

            if (literal(';')) {
                whitespace();
                continue;
            }
        }

        const auto delimiter = ignore_until(";}");
        if (delimiter != ';') break;

        i_++;
        whitespace();
    }

    return declarations;
}

std::optional<Selector> CSSParser::selector_node() {
    const auto first_tag = word();
    if (!first_tag) return std::nullopt;

    Selector out = TagSelector(to_lower(*first_tag));
    whitespace();

    while (i_ < s_.size() && s_[i_] != '{') {
        const auto tag = word();
        if (!tag) return std::nullopt;

        TagSelector descendant = TagSelector(to_lower(*tag));
        out = std::make_unique<DescendantSelector>(std::move(out), std::move(descendant));
        whitespace();
    }

    return out;
}

std::optional<CSSRule> CSSParser::rule() {
    whitespace();
    if (i_ >= s_.size()) return std::nullopt;

    auto selector = selector_node();
    if (!selector || !literal('{')) return std::nullopt;

    whitespace();
    auto declarations = parse_declarations();

    if (!literal('}')) return std::nullopt;

    return CSSRule{std::move(*selector), std::move(declarations)};
}

std::vector<CSSRule> CSSParser::parse() {
    std::vector<CSSRule> rules;

    while (i_ < s_.size()) {
        auto parsed_rule = rule();

        if (parsed_rule) {
            rules.push_back(std::move(*parsed_rule));
        } else {
            const auto delimiter = ignore_until("}");
            if (delimiter != '}') break;

            i_++;
            whitespace();
        }
    }

    return rules;
}
