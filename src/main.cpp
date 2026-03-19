#include <browser/browser.hpp>
#include <browser/url.hpp>
#include <cstdlib>
#include <iostream>
#include <print>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println(std::cerr, "ERROR: No arguments given.");
        return EXIT_FAILURE;
    }

    auto target = Url::parse_url(argv[1]);
    if (!target) {
        std::println(std::cerr, "{}", target.error());
        return EXIT_FAILURE;
    }

    Browser browser = {};
    auto result = browser.load(*target);
    if (!result) {
        std::println(std::cerr, "{}", result.error());
        return EXIT_FAILURE;
    }

    browser.run();

    return EXIT_SUCCESS;
}
