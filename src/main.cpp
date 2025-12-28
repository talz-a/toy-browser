#include <iostream>
#include <print>
#include <string_view>
#include "browser/browser.hpp"
#include "browser/url.hpp"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::println(std::cerr, "ERROR: No arguments given.");
        return -1;
    }

    try {
        url target{argv[1]};
        browser browser_instance{};
        browser_instance.load(target);
    } catch (const std::exception& e) {
        std::println(std::cerr, "ERROR {}", e.what());
        return -1;
    }

    return 0;
}
