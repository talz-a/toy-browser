#include <browser/browser.hpp>
#include <browser/url.hpp>
#include <iostream>
#include <print>
#include <span>
#include <string_view>

int main(int argc, char* argv[]) {
    auto args = std::span(argv, size_t(argc));

    if (args.size() < 2) {
        std::println(std::cerr, "ERROR: No arguments given.");
        return -1;
    }

    try {
        url target(args[1]);
        browser browser_instance{};
        browser_instance.load(target);
        return 0;
    } catch (const std::exception& e) {
        std::println(std::cerr, "Exception: {}", e.what());
        return 1;
    }
}
