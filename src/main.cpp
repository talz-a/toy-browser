#include <iostream>
#include <string_view>
#include "browser/browser.hpp"
#include "browser/url.hpp"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "ERROR: No arguments given.\n";
        return -1;
    }

    try {
        url target{argv[1]};
        browser browser_instance{};
        browser_instance.load(target);
        browser_instance.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << ".\n";
        return -1;
    }

    return 0;
}