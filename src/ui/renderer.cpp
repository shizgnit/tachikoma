#include "ui/renderer.hpp"
#include <thread>
#include <chrono>
#include <curses.h>

namespace tachikoma::ui {

void render_text(int y, int x, const std::string& text, Attr attr) {
    attr_t attrs = static_cast<attr_t>(attr);
    attrset(attrs);
    mvaddstr(y, x, text.c_str());
    attrset(A_NORMAL);
}

void render_colored(int y, int x, const std::string& text, int cp) {
    attr_t attrs = static_cast<attr_t>(COLOR_PAIR(cp));
    attrset(attrs);
    mvaddstr(y, x, text.c_str());
    attrset(A_NORMAL);
}

void render_bold(int y, int x, const std::string& text, int cp) {
    attrset(A_BOLD | COLOR_PAIR(cp));
    mvaddstr(y, x, text.c_str());
    attrset(A_NORMAL);
}

void fill_row(int y, int x, int width, int cp) {
    if (width <= 0) return;
    std::string cells(static_cast<size_t>(width), ' ');
    attrset(COLOR_PAIR(cp));
    mvaddstr(y, x, cells.c_str());
    attrset(A_NORMAL);
}

void draw_box_top(int y, int x, int width, int cp) {
    if (width < 2) return;
    if (cp >= 0) attrset(COLOR_PAIR(cp));
    mvaddch(y, x, '+');
    for (int i = 1; i < width - 1; ++i) mvaddch(y, x + i, '-');
    mvaddch(y, x + width - 1, '+');
    attrset(A_NORMAL);
}

void draw_box_bottom(int y, int x, int width, int cp) {
    draw_box_top(y, x, width, cp); // same shape on the bottom edge
}

void draw_box_side(int y_start, int x, int height, int cp) {
    if (height < 1) return;
    if (cp >= 0) attrset(COLOR_PAIR(cp));
    for (int i = 0; i < height; ++i) mvaddch(y_start + i, x, '|');
    attrset(A_NORMAL);
}

void render_char(int y, int x, char ch) {
    attrset(A_NORMAL);
    mvaddch(y, x, ch);
}

int screen_width() {
    return static_cast<int>(getmaxx(stdscr));
}

int screen_height() {
    return static_cast<int>(getmaxy(stdscr));
}

void clear_window() {
    werase(stdscr);
}

void render_hline(int y, int x, int width, char ch) {
    mvhline(y, x, ch, width);
}

void render_vline(int y, int x, int height, char ch) {
    mvvline(y, x, ch, height);
}

void render_box(int y_start, int x_start, int height, int width) {
    for (int i = 0; i < width; ++i) {
        mvaddch(y_start, x_start + i, '-');
        mvaddch(y_start + height - 1, x_start + i, '-');
    }
    for (int i = 1; i < height - 1; ++i) {
        mvaddch(y_start + i, x_start, '|');
        mvaddch(y_start + i, x_start + width - 1, '|');
    }
}

void render_scrolling_text(const std::string& text, int y, int x, int delay_ms) {
    int start_x = x;
    for (char c : text) {
        mvaddch(y, x, c);
        ++x;
        ::refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
}

int text_width(const std::string& text) {
    return static_cast<int>(text.length());
}

std::string truncate(const std::string& text, int max_width) {
    if (static_cast<int>(text.length()) <= max_width) {
        return text;
    }
    return text.substr(0, max_width - 3) + "...";
}

int center_x(const std::string& text, int term_width) {
    int text_w = text_width(text);
    return (term_width - text_w) / 2;
}

} // namespace tachikoma::ui
