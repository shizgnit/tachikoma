// Platform-facing layer.
// This file (terminal lifecycle), renderer.cpp (cell drawing) and input.cpp
// (key reading) are the ONLY translation units that call curses/PDCurses
// directly. All other UI code goes through ui:: helpers, so ncurses vs
// PDCurses differences stay contained in this one place.

#include "ui/terminal.hpp"
#include <cstdlib>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
namespace {
// PDCurses requires an allocated console screen buffer. Without one, initscr()
// prints "Unable to create SP" and terminates the process before we can react,
// so probe for a usable console first and fail with a clear message instead.
bool console_available() {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!handle) return false;
    CONSOLE_SCREEN_BUFFER_INFO info{};
    return ::GetConsoleScreenBufferInfo(handle, &info) != 0 &&
           info.dwSize.X > 0 && info.dwSize.Y > 0;
}
} // namespace
#endif

namespace tachikoma::ui {

Terminal::Terminal() = default;
Terminal::~Terminal() {
    if (initialized_) {
        shutdown();
    }
}

void Terminal::init() {
    // Set the escape-sequence timeout low BEFORE initscr(). Default is 1000ms,
    // longer than our getch timeout — so a timeout fired mid-arrow-key would
    // return bare ESC instead of KEY_UP/DOWN/LEFT/RIGHT. PDCurses reads
    // Windows virtual keys directly (no escape-sequence parsing) and has no
    // set_escdelay API, so this only applies to ncurses.
#ifndef PDCURSES
    set_escdelay(50);
#endif

#ifdef _WIN32
    if (!console_available()) {
        throw std::runtime_error(
            "no interactive console attached - run tachikoma from a cmd/PowerShell "
            "window (the PDCurses backend requires an allocated screen buffer)");
    }
#endif
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
