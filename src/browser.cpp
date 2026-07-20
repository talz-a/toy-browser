#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <browser/block_layout.hpp>
#include <browser/browser.hpp>
#include <browser/css_parser.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <browser/url.hpp>
#include <browser/utils.hpp>
#include <cmath>
#include <format>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <variant>
#include <vector>

namespace {
constexpr float SCROLL_STEP = 100.0f;

float window_display_scale(SDL_Window* window) {
    const float scale = SDL_GetWindowDisplayScale(window);
    return scale > 0.f ? scale : 1.f;
}

[[maybe_unused]] void print_tree(const HTMLNode& node, int indent = 0) {
    std::string_view current_tag;

    std::visit(match{
                   [&](const Text& text) {
                       std::print("{:>{}}", "", indent);
                       std::println("{}", text.text);
                   },
                   [&](const Element& element) {
                       std::print("{:>{}}", "", indent);
                       std::println("<{}>", element.tag);
                       current_tag = element.tag;
                   },
               },
               node.data);

    for (const auto& child : node.children) {
        print_tree(*child, indent + 2);
    }

    if (!current_tag.empty()) {
        std::print("{:>{}}", "", indent);
        std::println("</{}>", current_tag);
    }
}

[[maybe_unused]] void print_layout_tree(const BlockLayout& layout, int indent = 0) {
    std::print("{:>{}}BlockLayout", "", indent);

    std::visit(match{
                   [](const Element& element) { std::print(" (<{}>)", element.tag); },
                   [](const Text& text) { std::print(" (\"{}...\")", text.text.substr(0, 20)); },
               },
               layout.node().data);
    std::println();

    for (const auto& child : layout.children()) {
        print_layout_tree(*child, indent + 2);
    }
}

// @TODO: Move this to a better place.
void style(HTMLNode& node, const std::vector<CSSRule>& rules) {
    node.style.clear();

    for (const auto& [property, default_value] : INHERITED_PROPERTIES) {
        std::string key{property};
        std::string value{default_value};
        if (node.parent) {
            if (const auto inherited = node.parent->style.find(key);
                inherited != node.parent->style.end()) {
                value = inherited->second;
            }
        }
        node.style.emplace(std::move(key), std::move(value));
    }

    for (const auto& [selector, body] : rules) {
        if (matches_any(selector, node)) {
            for (const auto& [property, value] : body) {
                // std::println("DEBUG: Adding style {} {}.", property, value);
                node.style.insert_or_assign(property, value);
            }
        }
    }

    if (const auto* element = std::get_if<Element>(&node.data)) {
        if (const auto inline_style = element->attributes.find("style");
            inline_style != element->attributes.end()) {
            const auto pairs = CSSParser(inline_style->second).parse_declarations();
            for (const auto& [property, value] : pairs) {
                // std::println("DEBUG: Adding style {} {}.", property, value);
                node.style.insert_or_assign(property, value);
            }
        }
    }

    if (auto font_size = node.style.find("font-size");
        font_size != node.style.end() && font_size->second.ends_with("%")) {
        const std::string_view parent_font_size =
            node.parent ? std::string_view{node.parent->style.at("font-size")} : DEFAULT_FONT_SIZE;

        // @NOTE: stof is smart; it reads the digits and stops at non numeric characters.
        const float node_pct = std::stof(font_size->second) / 100.0f;

        // @NOTE: We don't want to read the px, but stof handles that automatically.
        const float parent_px = std::stof(std::string{parent_font_size});

        font_size->second = std::format("{}px", node_pct * parent_px);
    }

    for (const auto& child : node.children) {
        if (child) style(*child, rules);
    }
}

void paint_tree(BlockLayout* block, std::vector<DrawCmd>& display_list) {
    display_list.append_range(block->paint());

    if (block->layout_mode() == LayoutMode::Block) {
        for (const auto& child : block->children()) {
            paint_tree(child.get(), display_list);
        }
        return;
    }

    for (const auto& line : block->lines()) {
        for (const auto& word : line->children()) {
            display_list.append_range(word->paint());
        }
    }
}

void paint_tree(const LayoutParent& layout_object, std::vector<DrawCmd>& display_list) {
    std::visit(match{
                   [&](DocumentLayout* document) {
                       for (const auto& child : document->children()) {
                           paint_tree(child.get(), display_list);
                       }
                   },
                   [&](BlockLayout* block) { paint_tree(block, display_list); },
               },
               layout_object);
}

std::vector<std::string> find_stylesheets(HTMLNode& root) {
    std::vector<HTMLNode*> nodes;
    tree_to_list(root, nodes);

    std::vector<std::string> stylesheets;
    for (const auto* node : nodes) {
        const auto* element = std::get_if<Element>(&node->data);
        if (!element || element->tag != "link") continue;

        const auto rel = element->attributes.find("rel");
        const auto href = element->attributes.find("href");
        if (rel != element->attributes.end() && rel->second == "stylesheet" &&
            href != element->attributes.end()) {
            stylesheets.push_back(href->second);
        }
    }

    return stylesheets;
}

std::vector<CSSRule> load_stylesheets(const Url& base_url, HTMLNode& root) {
    std::vector<CSSRule> rules = CSSParser(read_file("assets/browser.css")).parse();

    for (const auto& link : find_stylesheets(root)) {
        const auto stylesheet_url = base_url.resolve(link);
        if (!stylesheet_url) {
            std::println(std::cerr,
                         "Failed to resolve stylesheet: {}",
                         url_error_message(stylesheet_url.error()));
            continue;
        }

        const auto body = stylesheet_url->request();
        if (!body) {
            std::println(
                std::cerr, "Failed to load stylesheet: {}", url_error_message(body.error()));
            continue;
        }

        auto stylesheet_rules = CSSParser(*body).parse();
        for (auto& rule : stylesheet_rules) {
            rules.push_back(std::move(rule));
        }
    }

    return rules;
}
}  // namespace

Browser::Browser() {
    float content_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (content_scale <= 0.f) {
        content_scale = 1.f;
    }
    const int win_w = static_cast<int>(std::lround(640.f * content_scale));
    const int win_h = static_cast<int>(std::lround(480.f * content_scale));

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("Toy Browser",
                                     win_w,
                                     win_h,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &window,
                                     &renderer)) {
        throw std::runtime_error(
            std::format("Couldn't create window/renderer: {}", SDL_GetError()));
    }
    window_.reset(window);
    renderer_.reset(renderer);

    text_engine_.reset(TTF_CreateRendererTextEngine(renderer_.get()));

    if (!text_engine_) {
        throw std::runtime_error(std::format("Couldn't create text engine: {}", SDL_GetError()));
    }
}

Browser::~Browser() = default;

std::expected<void, UrlError> Browser::load(const Url& url) {
    const auto body = url.request();

    if (!body) {
        return std::unexpected(body.error());
    }

    nodes_ = HTMLParser(*body).parse();
    auto css_rules = load_stylesheets(url, *nodes_);

    auto cascade_priority = [](const CSSRule& rule) { return selector_priority(rule.selector); };
    std::ranges::stable_sort(css_rules, std::less{}, cascade_priority);

    style(*nodes_, css_rules);

    // print_tree(*nodes_);

    reflow();

    // if (!document_->children().empty()) {
    //     print_layout_tree(*document_->children().front());
    // }
    return {};
}

void Browser::reflow() {
    if (!nodes_) return;

    int w, h;
    SDL_GetWindowSizeInPixels(window_.get(), &w, &h);
    const float scale = window_display_scale(window_.get());

    display_list_.clear();

    document_.emplace(nodes_.get(), text_engine_.get(), &font_cache_, static_cast<float>(w), scale);
    document_->layout();

    paint_tree(&*document_, display_list_);

    const float max_y = std::max(document_->height() - static_cast<float>(h), 0.0f);
    scroll_ = std::clamp(scroll_, 0.0f, max_y);
}

void Browser::process_events() {
    bool needs_reflow = false;
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            is_running_ = false;
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED ||
                   event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
                   event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) {
            needs_reflow = true;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_DOWN) {
                if (!document_) continue;

                int w, h;
                SDL_GetWindowSizeInPixels(window_.get(), &w, &h);
                const float scale = window_display_scale(window_.get());

                const float max_y = std::max(document_->height() - static_cast<float>(h), 0.0f);

                scroll_ = std::min(scroll_ + SCROLL_STEP * scale, max_y);

            } else if (event.key.key == SDLK_UP) {
                const float scale = window_display_scale(window_.get());
                scroll_ = std::max(0.f, scroll_ - SCROLL_STEP * scale);
            }
        }
    }

    if (needs_reflow) reflow();
}

void Browser::render() {
    SDL_SetRenderDrawColor(renderer_.get(), 255, 255, 255, 255);
    SDL_RenderClear(renderer_.get());

    int w, h;
    SDL_GetWindowSizeInPixels(window_.get(), &w, &h);

    for (const auto& cmd : display_list_) {
        std::visit(
            [&](const auto& command) {
                if (command.top() > scroll_ + static_cast<float>(h)) return;
                if (command.bottom() < scroll_) return;
                command.execute(scroll_, renderer_.get());
            },
            cmd);
    }

    SDL_RenderPresent(renderer_.get());
}

void Browser::run() {
    is_running_ = true;

    while (is_running_) {
        process_events();
        render();
        SDL_Delay(16);
    }
}
