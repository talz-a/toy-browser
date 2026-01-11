#include <iostream>
#include <span>
#include <string_view>
#include "browser/browser.hpp"
#include "browser/url.hpp"

int main(int argc, char* argv[]) {
    try {
        auto args = std::span(argv, size_t(argc));

        if (args.size() < 2) {
            std::cerr << "ERROR: No arguments given." << "\n";
            return -1;
        }

        url target{args[1]};
        browser browser_instance{};
        browser_instance.load(target);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return 1;
    }
}
