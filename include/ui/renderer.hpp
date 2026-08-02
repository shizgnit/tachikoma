#pragma once

#include <string>
#include <vector>
#include <ncurses.h>

namespace tachikoma::ui {

/// Text attributes for rendering
enum class Attr {
    Normal = 0,
    Bold = A_BOLD,
    Dim = A_DIM,
    Underline = A_UNDERLINE,
    Reverse = A_REVERSE,
};

/// Render a text string at a position
void render_text(int y, int x, const std::string& text, Attr attr = Attr::Normal);

/// Render a colored text string
void render_colored(int y, int x, const std::string& text, int color_pair);

/// Render a horizontal line
void render_hline(int y, int x, int width, char ch = '-');

/// Render a vertical line
void render_vline(int y, int x, int height, char ch = '|');

/// Render a box
void render_box(int y_start, int x_start, int height, int width);

/// Render scrolling text effect (typewriter style)
void render_scrolling_text(const std::string& text, int y, int x, int delay_ms = 30);

/// Get the width of a string in terminal columns
int text_width(const std::string& text);

/// Truncate text to fit width
std::string truncate(const std::string& text, int max_width);

/// Center text horizontally
int center_x(const std::string& text, int term_width);

} // namespace tachikoma::ui
