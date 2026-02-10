#include <SFML/Graphics.hpp>
#include <browser/draw_commands.hpp>

void draw_text::execute(float scroll, sf::RenderWindow& window) {
    text_.setPosition({left_, top_ - scroll});
    text_.setFillColor(sf::Color::Black);
    window.draw(text_);
}

void draw_rect::execute(float scroll, sf::RenderWindow& window) {
    sf::RectangleShape bg({right_ - left_, bottom_ - top_});
    bg.setPosition({left_, top_ - scroll});
    bg.setFillColor(color_);
    window.draw(bg);
}
