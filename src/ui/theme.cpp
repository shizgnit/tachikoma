// Theme layer: maps semantic colors to curses pairs.
// The palette follows btop's default (Nord) theme. Standard color slots are
// redefined via init_color() - a portable API present in both ncurses and
// PDCurses - so the exact RGB values land on any terminal that honors them,
// with a graceful fallback to the system 16-color set otherwise.

#include "ui/theme.hpp"
#include <array>
#include <curses.h>
#include <utility>

namespace tachikoma::ui {

namespace {

bool custom_colors{false};

int pair_id(ThemeColor c) {
    switch (c) {
        case ThemeColor::Font:      return 201; // white on slate-black background
        case ThemeColor::Title:     return 202; // frost cyan
        case ThemeColor::Hi:        return 203; // nord blue
        case ThemeColor::Border:    return 204; // dim slate
        case ThemeColor::Muted:     return 205; // dim slate (same hue, distinct slot)
        case ThemeColor::Ok:        return 206; // nord green
        case ThemeColor::Warn:      return 207; // nord yellow
        case ThemeColor::Crit:      return 208; // nord red
        case ThemeColor::SelText:   return 209; // white on slate (selection background)
        case ThemeColor::SelAccent: return 210; // frost on slate
    }
    return 201;
}

// Redefine one standard color slot to an exact RGB value.
bool define_color(short slot, int r8, int g8, int b8) {
    const short R = static_cast<short>(r8 * 1000 / 255);
    const short G = static_cast<short>(g8 * 1000 / 255);
    const short B = static_cast<short>(b8 * 1000 / 255);
    return init_color(slot, R, G, B) == OK;
}

// btop default theme (Nord palette), translucent variant:
//   font #D8DEE9, title/frost #8FBCBB, hi/blue #5E81AC, ok green #A3BE8C,
//   warn yellow #EBCB8B, crit red #BF616A - all as soft foregrounds over the
//   terminal's OWN background (no opaque canvas), so a translucent terminal
//   stays translucent and nothing clashes with the surrounding window.
//   Selection accent: dim slate #4C566A background for highlighted rows only.
const std::pair<short, std::array<int, 3>> kPalette[] = {
    {COLOR_WHITE,   {0xD8, 0xDE, 0xE9}},
    {COLOR_CYAN,    {0x8F, 0xBC, 0xBB}},
    {COLOR_BLUE,    {0x5E, 0x81, 0xAC}},
    {COLOR_MAGENTA, {0x4C, 0x56, 0x6A}}, // selection-row background accent
    {COLOR_GREEN,   {0xA3, 0xBE, 0x8C}},
    {COLOR_YELLOW,  {0xEB, 0xCB, 0x8B}},
    {COLOR_RED,     {0xBF, 0x61, 0x6A}},
};

} // namespace

void init_theme() {
    if (!has_colors()) return;

    for (const auto& [slot, rgb] : kPalette) {
        if (define_color(slot, rgb[0], rgb[1], rgb[2])) {
            custom_colors = true;
        }
    }

    // Semantic pairs. All regular text uses COLOR_DEFAULT (-1) as its
    // background so every cell inherits the terminal's native background -
    // one consistent color everywhere, translucent when the terminal is.
    // Only selection accents (209/210) carry an explicit background.
    init_pair(201, COLOR_WHITE, -1);
    init_pair(202, COLOR_CYAN, -1);
    init_pair(203, COLOR_BLUE, -1);
    init_pair(204, COLOR_MAGENTA, -1);
    init_pair(205, COLOR_MAGENTA, -1);
    init_pair(206, COLOR_GREEN, -1);
    init_pair(207, COLOR_YELLOW, -1);
    init_pair(208, COLOR_RED, -1);
    init_pair(209, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(210, COLOR_CYAN, COLOR_MAGENTA);

    // Deliberately NO bkgd(): the canvas keeps the terminal's own background
    // (translucent when the terminal is), instead of painting an opaque box.
}

int theme_pair(ThemeColor c) {
    return pair_id(c);
}

bool has_custom_colors() {
    return custom_colors;
}

} // namespace tachikoma::ui
