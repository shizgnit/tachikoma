#pragma once

#include <ncurses.h>
#include <optional>
#include <string>

namespace tachikoma::ui {

/// Key codes for special keys
enum class Key {
    Up = KEY_UP,
    Down = KEY_DOWN,
    Left = KEY_LEFT,
    Right = KEY_RIGHT,
    Enter = KEY_ENTER,
    Escape = 27,
    F1 = KEY_F(1),
    F5 = KEY_F(5),
    Q = 'q',
    Q_UPPER = 'Q'
};

/// Wait for and return the next key press
int wait_for_key();

/// Get the next key with timeout (returns nullopt on timeout)
std::optional<int> get_key_with_timeout(int timeout_ms);

/// Check if a key is ready (non-blocking)
bool key_ready();

/// Convert key code to string representation
std::string key_to_string(int key);

} // namespace tachikoma::ui
