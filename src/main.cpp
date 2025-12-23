#include <iostream>
#include <print>
#include <string_view>
#include "browser/browser.hpp"
#include "browser/url.hpp"

void show(std::string_view body) {
    bool in_tag = false;
    for (char c : body) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            std::print("{}", c);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "ERROR: No arguments given.\n";
        return -1;
    }

    try {
        url target{argv[1]};
        show(target.request());

        browser browser_instance{};
        browser_instance.load(target);
        browser_instance.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << ".\n";
        return -1;
    }

    return 0;
}