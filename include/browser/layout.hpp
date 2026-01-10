#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <variant>
#include <vector>

struct render_item {
    float x{}, y{};
    sf::Text text;
};

struct text_token {
    std::string text;
};

struct tag_token {
    std::string tag;
};

using token = std::variant<text_token, tag_token>;

class layout {
public:
    static constexpr float HSTEP = 13;
    static constexpr float VSTEP = 18;
    static constexpr int FONT_SIZE = 16;
    static constexpr unsigned int WIDTH = 800;

    layout(const std::vector<token>& tokens, const sf::Font& font);

    [[nodiscard]] const std::vector<render_item>& get_display_list() const { return display_list_; }

private:
    std::vector<render_item> display_list_;
    float cursor_x_ = HSTEP;
    float cursor_y_ = VSTEP;
    int weight_ = sf::Text::Style::Regular;
    int style_ = sf::Text::Style::Regular;
    int size_ = FONT_SIZE;

    // TODO: Change this.
    const sf::Font* font_;

    void process_token(const token& tok);
    void word(const std::string& word);
};
