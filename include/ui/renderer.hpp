#pragma once

#include <string>
#include <vector>
#include <curses.h>

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

/// Render bold colored text (selection rows)
void render_bold(int y, int x, const std::string& text, int color_pair);

/// Fill a horizontal run of cells with the given color pair's background
/// (used for full-row selection highlights and input blocks)
void fill_row(int y, int x, int width, int color_pair);

/// Draw a box top/bottom edge: +----...----+ (width >= 2)
void draw_box_top(int y, int x, int width, int color_pair = -1);
void draw_box_bottom(int y, int x, int width, int color_pair = -1);

/// Draw a single character with plain (unpaired) attributes
void render_char(int y, int x, char ch);

/// Render a horizontal line
void render_hline(int y, int x, int width, char ch = '-');

/// Render a vertical line
void render_vline(int y, int x, int height, char ch = '|');

/// Render a box
void render_box(int y_start, int x_start, int height, int width);

/// Render scrolling text effect (typewriter style)
void render_scrolling_text(const std::string& text, int y, int x, int delay_ms = 30);

/// Current screen dimensions (wraps getmaxx/getmaxy)
int screen_width();
int screen_height();

/// Clear the standard window (caller refreshes afterwards)
void clear_window();

/// Get the width of a string in terminal columns
int text_width(const std::string& text);

/// Truncate text to fit width
std::string truncate(const std::string& text, int max_width);

/// Center text horizontally
int center_x(const std::string& text, int term_width);

} // namespace tachikoma::ui
