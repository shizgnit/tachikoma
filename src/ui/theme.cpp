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

// btop default theme (Nord palette):
//   background #2E3440, font #D8DEE9, title/frost #8FBCBB, hi/blue #5E81AC,
//   border/dim/selected-bg #4C566A, ok green #A3BE8C, warn yellow #EBCB8B,
//   crit red #BF616A.
const std::pair<short, std::array<int, 3>> kPalette[] = {
    {COLOR_BLACK,   {0x2E, 0x34, 0x40}},
    {COLOR_WHITE,   {0xD8, 0xDE, 0xE9}},
    {COLOR_CYAN,    {0x8F, 0xBC, 0xBB}},
    {COLOR_BLUE,    {0x5E, 0x81, 0xAC}},
    {COLOR_MAGENTA, {0x4C, 0x56, 0x6A}},
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

    // Semantic pairs. Background slot (COLOR_BLACK) now holds the theme bg;
    // selection pairs use the dim-slate slot as their background.
    init_pair(201, COLOR_WHITE, COLOR_BLACK);
    init_pair(202, COLOR_CYAN, COLOR_BLACK);
    init_pair(203, COLOR_BLUE, COLOR_BLACK);
    init_pair(204, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(205, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(206, COLOR_GREEN, COLOR_BLACK);
    init_pair(207, COLOR_YELLOW, COLOR_BLACK);
    init_pair(208, COLOR_RED, COLOR_BLACK);
    init_pair(209, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(210, COLOR_CYAN, COLOR_MAGENTA);

    // Paint the whole canvas with the theme background (bkgd makes werase()
    // fill with it), so the app presents a full dark frame like btop.
    // Note: single-argument form - portable across ncurses and PDCurses.
    bkgd(static_cast<chtype>(' ') | static_cast<chtype>(COLOR_PAIR(201)));
}

int theme_pair(ThemeColor c) {
    return pair_id(c);
}

bool has_custom_colors() {
    return custom_colors;
}

} // namespace tachikoma::ui
