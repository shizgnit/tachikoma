#include "ui/terminal.hpp"
#include <cstdlib>

namespace tachikoma::ui {

Terminal::Terminal() = default;
Terminal::~Terminal() {
    if (initialized_) {
        shutdown();
    }
}

void Terminal::init() {
    // Set ESCDELAY low BEFORE initscr(). Default is 1000ms, which is longer
    // than our getch timeout — causing getch to return bare ESC instead of
    // KEY_UP/DOWN/LEFT/RIGHT when timeout fires before escape seq completes.
    set_escdelay(50);

    initscr();
    if (!stdscr) {
        throw std::runtime_error("Failed to initialize terminal (no TTY)");
    }

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    // Set timeout once at init — do NOT re-call timeout() in the main loop.
    timeout(200); // 200ms poll interval for background task results

    // Start color support
    if (has_colors()) {
        start_color();
        use_default_colors();

        // Initialize color pairs
        init_pair(1, COLOR_WHITE, -1);      // Default white
        init_pair(2, COLOR_GREEN, -1);      // Tachikoma green
        init_pair(3, COLOR_CYAN, -1);       // Cyan accent
        init_pair(4, COLOR_YELLOW, -1);     // Yellow warnings
        init_pair(5, COLOR_RED, -1);        // Red errors
        init_pair(6, COLOR_BLUE, -1);       // Blue info
        init_pair(7, COLOR_MAGENTA, -1);    // Magenta accent
        init_pair(8, COLOR_WHITE, COLOR_BLUE); // Selection highlight
        init_pair(9, COLOR_GREEN, COLOR_BLUE); // Green on blue
        init_pair(10, COLOR_BLACK, COLOR_GREEN); // Inverse green
    }

    initialized_ = true;
    refresh();
}

void Terminal::shutdown() {
    if (initialized_) {
        endwin();
        initialized_ = false;
    }
}

void Terminal::clear() {
    if (initialized_) {
        // Just erase the buffer - do NOT call ::refresh() here.
        // The caller will refresh() after drawing new content.
        // Calling refresh() after werase() pushes a blank frame causing flicker.
        werase(stdscr);
    }
}

void Terminal::refresh() {
    if (initialized_) {
        ::refresh();
    }
}

int Terminal::width() const {
    return getmaxx(stdscr);
}

int Terminal::height() const {
    return getmaxy(stdscr);
}

bool Terminal::is_initialized() const {
    return initialized_;
}

void Terminal::set_cursor_visible(bool visible) {
    curs_set(visible ? 1 : 0);
}

void Terminal::enable_raw_input() {
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
}

void Terminal::set_color_pair(int pair, int fg, int bg) {
    if (has_colors()) {
        init_pair(pair, fg, bg);
    }
}

TerminalGuard::TerminalGuard(Terminal& term) : terminal_(term) {}
TerminalGuard::~TerminalGuard() {
    terminal_.shutdown();
}

} // namespace tachikoma::ui
