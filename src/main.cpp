#include <browser/browser.hpp>
#include <browser/url.hpp>
#include <cstdlib>
#include <print>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println(std::cerr, "USAGE: {} <url>", argv[0]);
        return EXIT_FAILURE;
    }

    auto target = Url::parse(argv[1]);
    if (!target) {
        std::println(std::cerr, "ERROR: {}", url_error_message(target.error()));
        return EXIT_FAILURE;
    }

    Browser browser{};
    auto result = browser.load(*target);
    if (!result) {
        std::println(std::cerr, "ERROR: {}", url_error_message(result.error()));
        return EXIT_FAILURE;
    }

    browser.run();

    return EXIT_SUCCESS;
}