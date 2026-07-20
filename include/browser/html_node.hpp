#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using Attributes = std::unordered_map<std::string, std::string>;

struct Text {
    std::string text;
};

struct Element {
    std::string tag;
    Attributes attributes;
};

struct HTMLNode {
    std::variant<Text, Element> data;
    std::vector<std::unique_ptr<HTMLNode>> children;
    Attributes style;
    HTMLNode* parent = nullptr;
};
