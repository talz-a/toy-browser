#include <browser/browser.hpp>
#include <browser/url.hpp>
#include <SFML/Graphics.hpp>
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

    sf::Font loaded_font;
    if (!loaded_font.openFromFile("assets/Inter-VariableFont.ttf")) {
        std::println(std::cerr, "ERROR: No font loaded.");
        return EXIT_FAILURE;
    }

    Browser browser_instance = {};
    auto result = browser_instance.load(target.value());
    if (!result) {
        std::println(std::cerr, "{}", result.error());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
