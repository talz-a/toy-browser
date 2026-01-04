#include <exception>
#include <iostream>
#include <print>
#include <span>
#include <string_view>
#include "browser/browser.hpp"
#include "browser/url.hpp"

int main(int argc, char* argv[]) {  // NOLINT(bugprone-exception-escape)
    auto args = std::span(argv, size_t(argc));

    if (args.size() < 2) {
        std::println(std::cerr, "ERROR: No arguments given.");
        return -1;
    }

    try {
        url target{args[1]};
        browser browser_instance{};
        browser_instance.load(target);
    } catch (const std::exception& e) {
        std::println(std::cerr, "ERROR: {}", e.what());
        return -1;
    } catch (...) {
        std::println(std::cerr, "UNKNOWN ERROR");
        return -1;
    }

    return 0;
}
