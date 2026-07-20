#pragma once

#include <browser/document_layout.hpp>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
#include <browser/html_node.hpp>
#include <browser/sdl_raii.hpp>
#include <browser/url.hpp>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

class Browser {
public:
    Browser();
    ~Browser();

    Browser(const Browser&) = delete;
    Browser& operator=(const Browser&) = delete;
    Browser(Browser&&) = delete;
    Browser& operator=(Browser&&) = delete;

    [[nodiscard]] std::expected<void, UrlError> load(const Url& target_url);

    void run();

private:
    void reflow();
    void process_events();
    void render();

    float scroll_ = 0.0f;
    bool is_running_ = false;

    SDLRuntime sdl_runtime_;
    SDLWindowPtr window_;
    SDLRendererPtr renderer_;
    TTFRuntime ttf_runtime_;
    TTFTextEnginePtr text_engine_;
    FontCache font_cache_;
    std::unique_ptr<HTMLNode> nodes_;
    std::optional<DocumentLayout> document_;
    std::vector<DrawCmd> display_list_;
};
