#include <browser/browser.hpp>
#include <browser/url.hpp>
#include <cstdlib>
#include <print>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println(stderr, "USAGE: {} <url>", argv[0]);
        return EXIT_FAILURE;
    }

    auto target = Url::parse_url(argv[1]);
    if (!target) {
        std::println(stderr, "{}", target.error());
        return EXIT_FAILURE;
    }

    Browser browser = {};
    auto result = browser.load(*target);
    if (!result) {
        std::println(stderr, "{}", result.error());
        return EXIT_FAILURE;
    }

    browser.run();

    return EXIT_SUCCESS;
}
