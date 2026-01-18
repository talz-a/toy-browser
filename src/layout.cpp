#include "browser/layout.hpp"
#include <ranges>
#include "browser/constants.hpp"

// HACK: No way to get ascent of a word...
float layout::get_ascent(const sf::Font& font, unsigned int size) {
    if (size == 0) return 0.f;
    float top = font.getGlyph(U'\u00CA', size, false, 0).bounds.position.y;
    return -top;
}

// Hack: No way to get descent of a word...
float layout::get_descent(const sf::Font& font, unsigned int size) {
    if (size == 0) return 0.f;
    auto glyph = font.getGlyph('p', size, false);
    return glyph.bounds.size.y + glyph.bounds.position.y;
}

layout::layout(const std::vector<token>& tokens, const sf::Font& font) : font_(&font) {
    for (const auto& tok : tokens) {
        process_token(tok);
    }
    flush();
}

void layout::process_token(const token& tok) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, text_token>) {
                for (const auto w : std::views::split(arg.text, ' ')) {
                    if (w.empty()) continue;
                    word(std::ranges::to<std::string>(w));
                }
            } else if constexpr (std::is_same_v<T, tag_token>) {
                if (arg.tag == "i") {
                    style_ = sf::Text::Style::Italic;
                } else if (arg.tag == "/i") {
                    style_ = sf::Text::Style::Regular;
                } else if (arg.tag == "b") {
                    weight_ = sf::Text::Style::Bold;
                } else if (arg.tag == "/b") {
                    weight_ = sf::Text::Style::Regular;
                } else if (arg.tag == "small") {
                    size_ -= constants::font_size_small_diff;
                } else if (arg.tag == "/small") {
                    size_ += constants::font_size_small_diff;
                } else if (arg.tag == "big") {
                    size_ += constants::font_size_big_diff;
                } else if (arg.tag == "/big") {
                    size_ -= constants::font_size_big_diff;
                } else if (arg.tag == "br") {
                    flush();
                } else if (arg.tag == "/p") {
                    flush();
                    cursor_y_ += constants::v_step;
                }
            }
        },
        tok
    );
}

void layout::word(const std::string& word_text) {
    sf::Text word_sf(*font_, sf::String::fromUtf8(word_text.begin(), word_text.end()), size_);
    word_sf.setStyle(style_ | weight_);
    const auto word_width = word_sf.getGlobalBounds().size.x;

    sf::Text space_sf(*font_, " ", size_);
    const float space_width = space_sf.getGlobalBounds().size.x;

    if (cursor_x_ + word_width > constants::width - constants::h_step) {
        flush();
    }

    line_.push_back({cursor_x_, std::move(word_sf)});

    cursor_x_ += word_width + space_width;
}

void layout::flush() {
    if (line_.empty()) return;

    float max_ascent = 0.f;
    float max_descent = 0.f;

    for (const auto& [x, text] : line_) {
        const auto& font = text.getFont();
        unsigned int size = text.getCharacterSize();
        const float ascent = get_ascent(font, size);
        const float descent = get_descent(font, size);
        max_ascent = std::max(max_ascent, ascent);
        max_descent = std::max(max_descent, descent);
    }

    const float baseline = cursor_y_ + constants::line_height_multiplier * max_ascent;

    for (auto& [x, text] : line_) {
        const auto& font = text.getFont();
        unsigned int size = text.getCharacterSize();
        const float ascent = get_ascent(font, size);
        const float y = baseline - ascent;
        display_list_.emplace_back(render_item{.x = x, .y = y, .text = std::move(text)});
    }

    cursor_y_ = baseline + (constants::line_height_multiplier * max_descent);
    cursor_x_ = constants::h_step;
    line_.clear();
}
