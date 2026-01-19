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

browser::browser() : window_(sf::VideoMode({800, 600}), "Toy Browser") {
    if (!font_.openFromFile("assets/Inter-VariableFont.ttf")) {
        throw std::runtime_error("ERROR: No font loaded.");
    }
}

void browser::load(const url& target_url) {
    const auto body = target_url.request();
    nodes_ = html_parser(body).parse();

    // print_tree(nodes_);

    display_list_ =
        layout(nodes_, font_, static_cast<float>(window_.getSize().x)).get_display_list();
    run();
}

void browser::process_events() {
    bool needs_update = false;

    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window_.close();
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            sf::FloatRect visibleArea(
                {0.f, 0.f},
                {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)}
            );
            window_.setView(sf::View(visibleArea));

            // Need to handle window resize.
            needs_update = true;
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Down) {
                scroll_ += constants::scroll_step;
            } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                scroll_ -= constants::scroll_step;
            }

            // Need clamp logic.
            needs_update = true;
        }
    }

    if (needs_update && nodes_) {
        layout lay(nodes_, font_, static_cast<float>(window_.getSize().x));
        display_list_ = lay.get_display_list();

        float content_height = lay.get_height();
        float window_height = static_cast<float>(window_.getSize().y);

        // If content is shorter than window, scroll must be 0.
        float max_scroll = std::max(0.0f, content_height - window_height);
        scroll_ = std::clamp(scroll_, 0.0f, max_scroll);
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
