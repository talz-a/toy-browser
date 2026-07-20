#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <browser/draw_commands.hpp>
#include <browser/font_cache.hpp>
#include <browser/html_node.hpp>
#include <browser/sdl_raii.hpp>
#include <memory>
#include <variant>
#include <vector>

enum class LayoutMode {
    Block,
    InlineContext,
};

class DocumentLayout;
class BlockLayout;
class LineLayout;
class TextLayout;
using LayoutParent = std::variant<DocumentLayout*, BlockLayout*>;

class BlockLayout {
public:
    BlockLayout(const HTMLNode* node,
                LayoutParent parent,
                const BlockLayout* previous,
                TTF_TextEngine* text_engine,
                FontCache* font_cache,
                float scale);

    ~BlockLayout();

    void layout();

    [[nodiscard]] std::vector<DrawCmd> paint() const;
    [[nodiscard]] LayoutMode layout_mode() const;
    [[nodiscard]] const HTMLNode& node() const noexcept { return *node_; }
    [[nodiscard]] float x() const noexcept { return x_; }
    [[nodiscard]] float y() const noexcept { return y_; }
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }
    [[nodiscard]] const std::vector<std::unique_ptr<BlockLayout>>& children() const noexcept {
        return children_;
    }
    [[nodiscard]] const std::vector<std::unique_ptr<LineLayout>>& lines() const noexcept {
        return lines_;
    }

private:
    void new_line();
    void recurse(const HTMLNode& node);
    void word(const HTMLNode& node, const std::string& word);

    const HTMLNode* node_;
    LayoutParent parent_;
    const BlockLayout* previous_ = nullptr;
    float scale_;

    std::vector<std::unique_ptr<BlockLayout>> children_;
    std::vector<std::unique_ptr<LineLayout>> lines_;

    float cursor_x_{};

    float x_{};
    float y_{};
    float width_{};
    float height_{};

    FontCache* font_cache_;
    TTF_TextEngine* text_engine_;
};

class LineLayout {
public:
    LineLayout(BlockLayout* parent, LineLayout* previous);

    void layout();

    [[nodiscard]] float x() const noexcept { return x_; }
    [[nodiscard]] const std::vector<std::unique_ptr<TextLayout>>& children() const noexcept {
        return children_;
    }

private:
    friend class BlockLayout;

    BlockLayout* parent_;
    LineLayout* previous_;
    std::vector<std::unique_ptr<TextLayout>> children_;

    float x_{};
    float y_{};
    float width_{};
    float height_{};
};

class TextLayout {
public:
    TextLayout(TTF_Text* text, TTF_Font* font, LineLayout* parent, TextLayout* previous);

    ~TextLayout();

    void layout();

    [[nodiscard]] std::vector<DrawCmd> paint() const;

private:
    friend class LineLayout;

    TTFTextPtr text_;
    TTF_Font* font_;
    LineLayout* parent_;
    TextLayout* previous_;

    float x_{};
    float y_{};
    float width_{};
    float height_{};
};
