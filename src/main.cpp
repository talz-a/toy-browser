#include <iostream>
#include <memory>
#include <print>
#include <span>
#include <string_view>
// #include "browser/browser.hpp"
#include "browser/html_parser.hpp"
#include "browser/url.hpp"

void print_tree(const std::shared_ptr<node>& n, int indent = 0) {
    if (!n) return;

    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            std::print("{:>{}}", "", indent);
            if constexpr (std::is_same_v<T, text_data>) {
                std::println("{}", arg.text);
            } else if constexpr (std::is_same_v<T, element_data>) {
                std::println("<{}>", arg.tag);
            }
        },
        n->data
    );

    for (const auto& child : n->children) {
        print_tree(child, indent + 2);
    }
}

int main(int argc, char* argv[]) {
    auto args = std::span(argv, size_t(argc));

    if (args.size() < 2) {
        std::println(std::cerr, "ERROR: No arguments given.");
        return -1;
    }

    try {
        auto body = url(args[1]).request();
        auto nodes = html_parser(body).parse();
        print_tree(nodes);

        // browser browser_instance{};
        // browser_instance.load(target);
        return 0;
    } catch (const std::exception& e) {
        std::println(std::cerr, "Exception: {}", e.what());
        return 1;
    }
}
