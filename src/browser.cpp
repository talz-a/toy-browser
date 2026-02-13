#include <SFML/Graphics.hpp>
#include <browser/browser.hpp>
#include <browser/constants.hpp>
#include <browser/css_parser.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <exception>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

void print_tree(const html_node* n, int indent = 0) {
    if (!n) return;

    std::string_view current_tag;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            std::print("{:>{}}", "", indent);
            if constexpr (std::is_same_v<T, text_data>) {
                std::println("{}", arg.text);
            } else if constexpr (std::is_same_v<T, element_data>) {
                std::println("<{}>", arg.tag);
                current_tag = arg.tag;
            }
        },
        n->data
    );

    for (const auto& child : n->children) {
        print_tree(child.get(), indent + 2);
    }

    if (!current_tag.empty()) {
        std::print("{:>{}}", "", indent);
        std::println("</{}>", current_tag);
    }
}

void print_layout_tree(const block_layout* layout, int indent = 0) {
    if (!layout) return;

    std::print("{:>{}}", "", indent);
    std::print("BlockLayout");

    const html_node* n = layout->node_;
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, element_data>) {
                std::print(" (<{}>)", arg.tag);
            } else if constexpr (std::is_same_v<T, text_data>) {
                std::string snippet = arg.text.substr(0, 20);
                std::print(" (\"{}...\")", snippet);
            }
        },
        n->data
    );

    std::println("");

    for (const auto& child : layout->children_) {
        print_layout_tree(child.get(), indent + 2);
    }
}

void paint_tree(const layout_parent& layout_object, std::vector<draw_cmds>& display_list) {
    display_list.append_range(std::visit([](auto&& arg) { return arg->paint(); }, layout_object));

    for (const auto& child : std::visit(
             [](auto&& arg) -> const std::vector<std::unique_ptr<block_layout>>& {
                 return arg->children_;
             },
             layout_object
         )) {
        paint_tree(child.get(), display_list);
    }
}

// @TODO: Move this to a better place.
void style(html_node& node, const std::vector<css_rule>& rules) {
    node.style.clear();

    for (const auto& [selector, body] : rules) {
        if (matches_any(selector, node)) {
            for (const auto& [property, value] : body) {
                std::println("adding this style {} {}", property, value);
                node.style[property] = value;
            }
        }
    }

    auto* el = std::get_if<element_data>(&node.data);
    if (el && el->attributes.contains("style")) {
        auto pairs = css_parser(el->attributes["style"]).body();
        for (const auto& [property, value] : pairs) {
            std::println("Adding value {} {}", property, value);
            node.style[property] = value;
        }
    }

    for (const auto& child : node.children) {
        if (child) style(*child, rules);
    }
}

browser::browser() : window_(sf::VideoMode({800, 600}), "Toy Browser") {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// @TODO: This should be templated to work on both html and layout trees.
// Also not really sure why this takes the list as a param and does not just return one?
void tree_to_list(html_node& tree, std::vector<html_node*>& list) {
    list.push_back(&tree);
    for (const auto& child : tree.children) {
        tree_to_list(*child, list);
    }
}

void browser::load(const url& target_url) {
    const auto body = target_url.request();
    nodes_ = html_parser(body).parse();

    std::string default_css = read_file("assets/browser.css");
    std::vector<css_rule> css_rules = css_parser(default_css).parse();

    std::vector<html_node*> node_list;
    tree_to_list(*nodes_, node_list);

    std::vector<std::string> links;
    for (const auto& node : node_list) {
        auto* el = std::get_if<element_data>(&node->data);
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
        auto style_url = target_url.resolve(link);
        try {
            auto request_body = style_url.request();
            std::vector<css_rule> new_rules = css_parser(request_body).parse();
            css_rules.append_range(new_rules | std::views::as_rvalue);
        } catch (const std::exception& e) {
            continue;
        }
    }

    style(*nodes_, css_rules);

    // Debug print;
    // print_tree(nodes_.get());

    document_.emplace(
        document_layout(nodes_.get(), font_, static_cast<float>(window_.getSize().x))
    );

    document_->layout();

    // Debug print;
    // print_layout_tree(document_->children_.empty() ? nullptr :
    // document_->children_.front().get());

    display_list_.clear();
    paint_tree(&*document_, display_list_);

    run();
}

void browser::process_events() {
    bool needs_resize = false;

    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea(
                {0.f, 0.f},
                {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)}
            );
            window_.setView(sf::View(visibleArea));
            needs_resize = true;
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                if (!document_) return;

                float max_y = std::max(
                    document_->height_ + 2 * constants::v_step -
                        static_cast<float>(window_.getSize().y),
                    0.0f
                );
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

void browser::render() {
    window_.clear(sf::Color::White);

    for (auto& cmd : display_list_) {
        std::visit(
            [&](auto&& arg) {
                if (arg.top_ > scroll_ + static_cast<float>(window_.getSize().y)) return;
                if (arg.bottom_ < scroll_) return;
                arg.execute(scroll_, window_);
            },
            cmd
        );
    }

    window_.display();
}

void browser::run() {
    while (window_.isOpen()) {
        process_events();
        render();
    }
}
