#include "browser/browser.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <print>
#include <vector>
#include "browser/html_parser.hpp"

void print_tree(const std::shared_ptr<node>& n, int indent = 0) {
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
        print_tree(child, indent + 2);
    }

    if (!current_tag.empty()) {
        std::print("{:>{}}", "", indent);
        std::println("</{}>", current_tag);
    }
}

browser::browser()
    : window_{sf::VideoMode({constants::width, constants::height}), constants::window_title} {
    if (!font_.openFromFile(constants::default_font_path)) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

void browser::load(const url& target_url) {
    auto body = target_url.request();
    nodes_ = html_parser(body).parse();

    // print_tree(nodes_);

    display_list_ = layout(nodes_, font_).get_display_list();
    run();
}

void browser::process_events() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                scroll_ += constants::scroll_step;
            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ -= constants::scroll_step;
            }
        }
    }
}

void browser::render() {
    window_.clear(sf::Color::White);

    for (auto& [x, y, word] : display_list_) {
        if (y > scroll_ + constants::height) continue;
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
