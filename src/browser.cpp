#include <SDL3/SDL_events.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <browser/browser.hpp>
#include <browser/constants.hpp>
#include <browser/css_parser.hpp>
#include <browser/draw_commands.hpp>
#include <browser/html_parser.hpp>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

void print_tree(const HTMLNode& n, int indent = 0) {
    std::string_view current_tag;

    std::visit(
        [&]<typename T>(const T& arg) {
            std::print("{:>{}}", "", indent);
            if constexpr (std::is_same_v<T, Text>) {
                std::println("{}", arg.text);
            } else if constexpr (std::is_same_v<T, Element>) {
                std::println("<{}>", arg.tag);
                current_tag = arg.tag;
            }
        },
        n.data);

    for (const auto& child : n.children) {
        if (child) print_tree(*child, indent + 2);
    }

    if (!current_tag.empty()) {
        std::print("{:>{}}", "", indent);
        std::println("</{}>", current_tag);
    }
}

void print_layout_tree(const BlockLayout& layout, int indent = 0) {
    std::print("{:>{}}", "", indent);
    std::print("BlockLayout");

    const HTMLNode* n = layout.node_;
    std::visit(
        [&]<typename T>(const T& arg) {
            if constexpr (std::is_same_v<T, Element>) {
                std::print(" (<{}>)", arg.tag);
            } else if constexpr (std::is_same_v<T, Text>) {
                std::string snippet = arg.text.substr(0, 20);
                std::print(" (\"{}...\")", snippet);
            }
        },
        n->data);

    std::println("");

    for (const auto& child : layout.children_) {
        if (child) print_layout_tree(*child, indent + 2);
    }
}

// @TODO: Move this to a better place.
void style(HTMLNode& node, const std::vector<CSSRule>& rules) {
    node.style.clear();

    for (const auto& [prop, default_val] : INHERITED_PROPERTIES) {
        std::string key{prop};
        if (node.parent && node.parent->style.contains(key)) {
            node.style[key] = node.parent->style[key];
        } else {
            node.style[key] = std::string(default_val);
        }
    }

    for (const auto& [selector, body] : rules) {
        if (matches_any(selector, node)) {
            for (const auto& [property, value] : body) {
                // std::println("DEBUG: Adding style {} {}.", property, value);
                node.style[property] = value;
            }
        }
    }

    auto* el = std::get_if<Element>(&node.data);
    if (el && el->attributes.contains("style")) {
        auto pairs = CSSParser(el->attributes["style"]).body();
        for (const auto& [property, value] : pairs) {
            // std::println("DEBUG: Adding style {} {}.", property, value);
            node.style[property] = value;
        }
    }

    if (node.style.contains("font-size") && node.style["font-size"].ends_with("%")) {
        std::string parent_font_size;

        if (node.parent) {
            parent_font_size = node.parent->style["font-size"];
        } else {
            parent_font_size = INHERITED_PROPERTIES.at("font-size");
        }

        // @NOTE: stof is smart; it reads the digits and stops at non numeric characters.
        float node_pct = std::stof(node.style["font-size"]) / 100.0f;

        // @NOTE: We don't want to read the px, but stof handles that automatically.
        float parent_px = std::stof(parent_font_size);

        node.style["font-size"] = std::format("{}px", node_pct * parent_px);
    }

    for (const auto& child : node.children) {
        if (child) style(*child, rules);
    }
}

// @TODO: Move this to a better place.
void paint_tree(BlockLayout* block, std::vector<DrawCmd>& display_list) {
    display_list.append_range(block->paint());

    if (block->get_layout_mode() == LayoutMode::Block) {
        for (const auto& child : block->children_) {
            paint_tree(child.get(), display_list);
        }
        return;
    }

    for (auto& line : block->lines_) {
        display_list.append_range(line->paint());
        for (auto& word : line->children_) {
            display_list.append_range(word->paint());
        }
    }
}

void paint_tree(const LayoutParent& layout_object, std::vector<DrawCmd>& display_list) {
    std::visit(
        [&]<typename T>(T* arg) {
            if constexpr (std::is_same_v<T, DocumentLayout>) {
                display_list.append_range(arg->paint());
                for (const auto& child : arg->children_) {
                    paint_tree(child.get(), display_list);
                }
            } else {
                static_assert(std::is_same_v<T, BlockLayout>);
                paint_tree(arg, display_list);
            }
        },
        layout_object);
}

// @TODO: Move away from throwing here.
Browser::Browser() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::format("Couldn't initialize SDL: {}", SDL_GetError()));
    }

    if (!SDL_CreateWindowAndRenderer("Toy Browser",
                                     640,
                                     480,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &window_,
                                     &renderer_)) {
        SDL_Quit();
        throw std::runtime_error(
            std::format("Couldn't create window/renderer: {}", SDL_GetError()));
    }

    if (!TTF_Init()) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(std::format("Couldn't initialize SDL_ttf: {}", SDL_GetError()));
    }

    text_engine_ = TTF_CreateRendererTextEngine(renderer_);

    if (!text_engine_) {
        TTF_Quit();
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(std::format("Couldn't create text engine: {}", SDL_GetError()));
    }
}

Browser::~Browser() {
    // 1. Clean up all the TTF_Text pointers in the display list
    for (auto& cmd : display_list_) {
        if (auto* draw_text = std::get_if<DrawTextCmd>(&cmd)) {
            TTF_DestroyText(draw_text->text_);
        }
    }
    display_list_.clear();

    // 2. Clear any other SDL_ttf resources before shutting down.
    document_.reset();
    font_cache_.clear();

    // 3. Destroy the text engine (must happen before destroying the renderer)
    if (text_engine_) {
        TTF_DestroyRendererTextEngine(text_engine_);
    }

    // 4. Quit the TTF subsystem
    TTF_Quit();

    // 5. Destroy core SDL resources
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);

    // 6. Quit SDL
    SDL_Quit();
}

std::expected<void, std::string> Browser::load(const Url& url) {
    const auto body = url.request();

    if (!body) return std::unexpected(body.error());

    nodes_ = HTMLParser(*body).parse();

    std::string default_css = read_file("assets/browser.css");
    std::vector<CSSRule> css_rules = CSSParser(default_css).parse();

    std::vector<HTMLNode*> node_list;
    tree_to_list(*nodes_, node_list);

    std::vector<std::string> links;
    for (const auto& node : node_list) {
        auto* el = std::get_if<Element>(&node->data);
        if (el && el->tag == "link") {
            auto it_rel = el->attributes.find("rel");
            auto it_href = el->attributes.find("href");
            if (it_rel != el->attributes.end() && it_rel->second == "stylesheet" &&
                it_href != el->attributes.end()) {
                links.push_back(it_href->second);
            }
        }
    }

    for (const auto& link : links) {
        auto style_url = url.resolve(link);

        if (!style_url) {
            std::println(std::cerr, "Failed to fetch stylesheet: {}", style_url.error());
            continue;
        }

        auto request_body = style_url.value().request();

        if (!request_body) {
            std::println(std::cerr, "Failed to load stylesheet: {}", request_body.error());
            continue;
        }

        std::vector<CSSRule> new_rules = CSSParser(*request_body).parse();
        for (auto& rule : new_rules) {
            css_rules.push_back(std::move(rule));
        }
    }

    auto cascade_priority = [](const CSSRule& rule) { return selector_priority(rule.first); };
    std::ranges::stable_sort(css_rules, std::less{}, cascade_priority);

    style(*nodes_, css_rules);

    // Debug print;
    // print_tree(*nodes_);

    // Debug print;
    // if (!document_->children_.empty()) {
    //     print_layout_tree(*document_->children_.front());
    // }

    reflow();
    return {};
}

void Browser::reflow() {
    if (!nodes_) return;

    // 1. Determine new bounds
    int w, h;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    const float scale = SDL_GetWindowPixelDensity(window_);

    // 2. Manual Cleanup: Destroy C-style text resources
    // This is the "inline" version of your display list cleanup
    for (auto& cmd : display_list_) {
        if (auto* draw_text = std::get_if<DrawTextCmd>(&cmd)) {
            if (draw_text->text_) {
                TTF_DestroyText(draw_text->text_);
            }
        }
    }
    display_list_.clear();

    // 3. Recalculate Layout
    // emplace() calls the DocumentLayout constructor in-place
    document_.emplace(nodes_.get(), text_engine_, &font_cache_, static_cast<float>(w), scale);
    document_->layout();

    // 4. Re-paint the tree into the now-empty display_list_
    paint_tree(&*document_, display_list_);

    // 5. Clamp scroll_ to the new document height
    float max_y =
        std::max(document_->height_ + 2 * constants::v_step * scale - static_cast<float>(h), 0.0f);
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
                SDL_GetWindowSizeInPixels(window_, &w, &h);
                const float scale = SDL_GetWindowPixelDensity(window_);

                float max_y = std::max(
                    document_->height_ + 2 * constants::v_step * scale - static_cast<float>(h),
                    0.0f);

                scroll_ = std::min(scroll_ + constants::scroll_step * scale, max_y);

            } else if (event.key.key == SDLK_UP) {
                const float scale = SDL_GetWindowPixelDensity(window_);
                scroll_ = std::max(0.f, scroll_ - constants::scroll_step * scale);
            }
        }
    }

    if (needs_reflow) reflow();
}

void Browser::render() {
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderClear(renderer_);

    int w, h;
    SDL_GetWindowSizeInPixels(window_, &w, &h);

    for (auto& cmd : display_list_) {
        std::visit(
            [&](auto&& arg) {
                if (arg.top_ > scroll_ + static_cast<float>(h)) return;
                if (arg.bottom_ < scroll_) return;
                arg.execute(scroll_, renderer_);
            },
            cmd);
    }

    SDL_RenderPresent(renderer_);
}

void Browser::run() {
    is_running_ = true;

    while (is_running_) {
        process_events();
        render();
        SDL_Delay(16);
    }
}
