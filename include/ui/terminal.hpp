#pragma once

#include <curses.h>
#include <string>
#include <functional>

namespace tachikoma::ui {

/// Terminal manager - handles initialization and cleanup
class Terminal {
public:
    Terminal();
    ~Terminal();

    // Non-copyable
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    /// Initialize the terminal
    void init();

    /// Shutdown the terminal
    void shutdown();

    /// Clear the screen
    void clear();

    /// Refresh the display
    void refresh();

    /// Get terminal width
    int width() const;

    /// Get terminal height
    int height() const;

    /// Check if terminal is initialized
    bool is_initialized() const;

    /// Set cursor visibility
    void set_cursor_visible(bool visible);

    /// Enable raw input mode
    void enable_raw_input();

    /// Set background color pair
    void set_color_pair(int pair, int fg, int bg);

private:
    bool initialized_{false};
};

/// RAII wrapper for terminal operations
class TerminalGuard {
public:
    explicit TerminalGuard(Terminal& term);
    ~TerminalGuard();

private:
    Terminal& terminal_;
};

} // namespace tachikoma::ui
