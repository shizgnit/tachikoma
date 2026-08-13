#include "ui/input.hpp"
#include <ncurses.h>
#include <sstream>

namespace tachikoma::ui {

int wait_for_key() {
    return getch();
}

std::optional<int> get_key_with_timeout(int timeout_ms) {
    // Do NOT call timeout() here — it's set once during terminal init.
    // Re-calling timeout() on every loop iteration can reset ncurses'
    // internal escape-sequence parsing state, breaking arrow keys.
    int ch = getch();

    if (ch == ERR) {
        return std::nullopt;
    }
    return ch;
}

bool key_ready() {
    nodelay(stdscr, TRUE);
    int ch = getch();
    nodelay(stdscr, FALSE);

    if (ch == ERR) {
        return false;
    }

    // Push the character back
    ungetch(ch);
    return true;
}

std::string key_to_string(int key) {
    switch (key) {
        case KEY_UP: return "↑";
        case KEY_DOWN: return "↓";
        case KEY_LEFT: return "←";
        case KEY_RIGHT: return "→";
        case KEY_ENTER: return "Enter";
        case 27: return "Esc";
        case 'q':
        case 'Q': return "q (quit)";
        default:
            if (key >= 32 && key < 127) {
                std::string s;
                s += static_cast<char>(key);
                return s;
            }
            return "<" + std::to_string(key) + ">";
    }
}

} // namespace tachikoma::ui
