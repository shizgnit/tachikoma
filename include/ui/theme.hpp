#pragma once

namespace tachikoma::ui {

/// Semantic theme colors (btop-style "Nord" palette).
/// Each maps to a curses color pair resolved by theme_pair().
enum class ThemeColor {
    Font,       // main text on background
    Title,      // frost accent - box titles, values, symlinks
    Hi,         // blue accent - shortcuts, links, bar fills
    Border,     // dim slate - borders and dividers
    Muted,      // inactive/dim text (same hue as border)
    Ok,         // green - success / idle state
    Warn,       // yellow - warnings, suggestions
    Crit,       // red - errors
    SelText,    // selected-row text (bold white on slate background)
    SelAccent   // selected-row accents (frost on slate background)
};

/// Initialize the theme palette. Must be called after Terminal::init() so
/// color support is established. Safe to call when colors are unavailable -
/// pairs then resolve to the terminal's default 16-color set.
void init_theme();

/// Color pair id for a semantic theme color (for use with attrset/render helpers).
int theme_pair(ThemeColor c);

/// True if the standard colors were actually redefined to the exact RGB values.
bool has_custom_colors();

} // namespace tachikoma::ui
