#include "browser/layout.hpp"
#include <ranges>

layout::layout(const std::vector<token>& tokens, const sf::Font& font) : font_(&font) {
    for (const auto& tok : tokens) {
        process_token(tok);
    }
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
                    size_ -= 2;
                } else if (arg.tag == "/small") {
                    size_ += 2;
                } else if (arg.tag == "big") {
                    size_ += 4;
                } else if (arg.tag == "/big") {
                    size_ -= 4;
                }
            }
        },
        tok);
}

void layout::word(const std::string& word_text) {
    sf::Text word_sf(*font_, word_text, size_);
    word_sf.setStyle(style_ | weight_);
    const auto word_width = word_sf.getGlobalBounds().size.x;

    sf::Text space_sf(*font_, " ", size_);
    const float space_width = space_sf.getGlobalBounds().size.x;

    if (cursor_x_ + word_width > WIDTH - HSTEP) {
        cursor_y_ += font_->getLineSpacing(size_) * 1.25f;
        cursor_x_ = HSTEP;
    }

    display_list_.push_back({cursor_x_, cursor_y_, std::move(word_sf)});

    cursor_x_ += word_width + space_width;
}
