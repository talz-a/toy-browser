#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <print>
#include <vector>
#include "browser/html_parser.hpp"

void print_tree(const node* n, int indent = 0) {
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

    const node* n = layout->get_node();
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

    for (const auto& child : layout->get_children()) {
        print_layout_tree(child.get(), indent + 2);
    }
}

browser::browser() : window_(sf::VideoMode({800, 600}), "Toy Browser") {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

void browser::load(const url& target_url) {
    const auto body = target_url.request();
    nodes_ = html_parser(body).parse();

    // print_tree(nodes_.get());

    document_.emplace(
        document_layout(nodes_.get(), font_, static_cast<float>(window_.getSize().x))
    );
    document_->layout();

    // print_layout_tree(document_->get_root());

    display_list_ = document_->get_display_list();

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
                scroll_ += constants::scroll_step;
            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ -= constants::scroll_step;
            }
        }
    }

    if (needs_resize) {
        document_.emplace(nodes_.get(), font_, static_cast<float>(window_.getSize().x));
        document_->layout();
        display_list_ = document_->get_display_list();
    }
}

void browser::render() {
    window_.clear(sf::Color::White);

    for (auto& [x, y, word] : display_list_) {
        if (y > scroll_ + static_cast<float>(window_.getSize().y)) continue;
        if (y + constants::v_step < scroll_) continue;
        word.setPosition({x, y - scroll_});
        word.setFillColor(sf::Color::Black);
        window_.draw(word);
    }

    window_.display();
}

void browser::run() {
    while (window_.isOpen()) {
        process_events();
        render();
    }
}
