#include <SFML/Graphics.hpp>
#include <browser/browser.hpp>
#include <browser/constants.hpp>
#include <browser/css_parser.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <variant>
#include <vector>

void print_tree(const HTMLNode& n, int indent = 0) {
    std::string_view current_tag;

    std::visit(
        [&]<typename T>(const T& arg) {
            std::print("{:>{}}", "", indent);
            if constexpr (std::is_same_v<T, Text>) {
                std::println("{}", arg.text);
            } else if constexpr (std::is_same_v<T, Element>) {
                std::println("<{}>", arg.tag);
                current_tag = arg.tag;
            }
        },
        n.data);

    for (const auto& child : n.children) {
        if (child) print_tree(*child, indent + 2);
    }

    if (!current_tag.empty()) {
        std::print("{:>{}}", "", indent);
        std::println("</{}>", current_tag);
    }
}

void print_layout_tree(const BlockLayout& layout, int indent = 0) {
    std::print("{:>{}}", "", indent);
    std::print("BlockLayout");

    const HTMLNode* n = layout.node_;
    std::visit(
        [&]<typename T>(const T& arg) {
            if constexpr (std::is_same_v<T, Element>) {
                std::print(" (<{}>)", arg.tag);
            } else if constexpr (std::is_same_v<T, Text>) {
                std::string snippet = arg.text.substr(0, 20);
                std::print(" (\"{}...\")", snippet);
            }
        },
        n->data);

    std::println("");

    for (const auto& child : layout.children_) {
        if (child) print_layout_tree(*child, indent + 2);
    }
}

void paint_tree(const LayoutParent& layout_object, std::vector<draw_cmds>& display_list) {
    display_list.append_range(std::visit([](auto&& arg) { return arg->paint(); }, layout_object));

    for (const auto& child : std::visit(
             [](auto&& arg) -> const std::vector<std::unique_ptr<BlockLayout>>& {
                 return arg->children_;
             },
             layout_object)) {
        paint_tree(child.get(), display_list);
    }
}

// @TODO: Move this to a better place.
void style(HTMLNode& node, const std::vector<CSSRule>& rules) {
    node.style.clear();

    // Apply INHERITED_PROPERTIES.
    for (const auto& [prop, default_val] : INHERITED_PROPERTIES) {
        std::string key{prop};
        if (node.parent && node.parent->style.contains(key)) {
            node.style[key] = node.parent->style[key];
        } else {
            node.style[key] = std::string(default_val);
        }
    }

    for (const auto& [selector, body] : rules) {
        if (matches_any(selector, node)) {
            for (const auto& [property, value] : body) {
                // std::println("DEBUG: Adding style {} {}.", property, value);
                node.style[property] = value;
            }
        }
    }

    auto* el = std::get_if<Element>(&node.data);
    if (el && el->attributes.contains("style")) {
        auto pairs = CSSParser(el->attributes["style"]).body();
        for (const auto& [property, value] : pairs) {
            // std::println("DEBUG: Adding style {} {}.", property, value);
            node.style[property] = value;
        }
    }

    if (node.style.contains("font-size") && node.style["font-size"].ends_with("%")) {
        std::string parent_font_size;

        if (node.parent) {
            parent_font_size = node.parent->style["font-size"];
        } else {
            parent_font_size = INHERITED_PROPERTIES.at("font-size");
        }

        // @NOTE: stof is smart; it reads the digits and stops at non numeric characters.
        float node_pct = std::stof(node.style["font-size"]) / 100.0f;

        // @NOTE: We don't want to read the px, but stof handles that automatically.
        float parent_px = std::stof(parent_font_size);

        node.style["font-size"] = std::format("{}px", node_pct * parent_px);
    }

    for (const auto& child : node.children) {
        if (child) style(*child, rules);
    }
}

// @TODO: This should be templated to work on both html and layout trees.
// Also not really sure why this takes the list as a param and does not just return one?
void tree_to_list(HTMLNode& tree, std::vector<HTMLNode*>& list) {
    list.push_back(&tree);
    for (const auto& child : tree.children) {
        tree_to_list(*child, list);
    }
}

Browser::Browser() : window_(sf::VideoMode({800, 600}), "Toy Browser") {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
        // It is okay to throw here since this is a fatal error.
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

std::expected<void, std::string> Browser::load(const Url& url) {
    const auto body = url.request();

    if (!body) return std::unexpected(body.error());

    nodes_ = HTMLParser(body.value()).parse();

    std::string default_css = read_file("assets/browser.css");
    std::vector<CSSRule> css_rules = CSSParser{.s_ = default_css}.parse();

    std::vector<HTMLNode*> node_list;
    tree_to_list(*nodes_, node_list);

    std::vector<std::string> links;
    for (const auto& node : node_list) {
        auto* el = std::get_if<Element>(&node->data);
        if (el && el->tag == "link") {
            auto it_rel = el->attributes.find("rel");
            auto it_href = el->attributes.find("href");
            if (it_rel != el->attributes.end() && it_rel->second == "stylesheet" &&
                it_href != el->attributes.end()) {
                links.push_back(it_href->second);
            }
        }
    }

    for (const auto& link : links) {
        auto style_url = url.resolve(link);

        if (!style_url) {
            std::println(std::cerr, "Failed to fetch stylesheet: {}", style_url.error());
            continue;
        }

        auto request_body = style_url.value().request();

        if (!request_body) {
            std::println(std::cerr, "Failed to load stylesheet: {}", request_body.error());
            continue;
        }

        std::vector<CSSRule> new_rules = CSSParser(request_body.value()).parse();
        for (auto& rule : new_rules) {
            css_rules.push_back(std::move(rule));
        }
    }

    auto cascade_priority = [](const CSSRule& rule) { return selector_priority(rule.first); };
    std::ranges::stable_sort(css_rules, std::less{}, cascade_priority);

    style(*nodes_, css_rules);

    // Debug print;
    // print_tree(*nodes_);

    document_.emplace(DocumentLayout(nodes_.get(), font_, static_cast<float>(window_.getSize().x)));

    document_->layout();

    // Debug print;
    // if (!document_->children_.empty()) {
    //     print_layout_tree(*document_->children_.front());
    // }

    display_list_.clear();
    paint_tree(&*document_, display_list_);

    return {};
}

void Browser::process_events() {
    bool needs_resize = false;

    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea(
                {0.f, 0.f},
                {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)});

            window_.setView(sf::View(visibleArea));
            needs_resize = true;
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                if (!document_) return;

                float max_y = std::max(document_->height_ + 2 * constants::v_step -
                                           static_cast<float>(window_.getSize().y),
                                       0.0f);
                scroll_ = std::min(scroll_ + constants::scroll_step, max_y);

            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ = std::max(0.f, scroll_ - constants::scroll_step);
            }
        }
    }

    if (needs_resize) {
        document_.emplace(nodes_.get(), font_, static_cast<float>(window_.getSize().x));
        document_->layout();

        display_list_.clear();

        if (document_) paint_tree(&*document_, display_list_);
    }
}

void Browser::render() {
    window_.clear(sf::Color::White);

    for (auto& cmd : display_list_) {
        std::visit(
            [&](auto&& arg) {
                if (arg.top_ > scroll_ + static_cast<float>(window_.getSize().y)) return;
                if (arg.bottom_ < scroll_) return;
                arg.execute(scroll_, window_);
            },
            cmd);
    }

    window_.display();
}

void Browser::run() {
    while (window_.isOpen()) {
        process_events();
        render();
    }
}
