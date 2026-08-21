#include "ui/task_bar.hpp"
#include "filesystem/entry.hpp"
#include "ui/renderer.hpp"
#include "ui/theme.hpp"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

namespace tachikoma::ui {

void TaskBar::set_viewport(int y) {
    y_ = y;
}

void TaskBar::set_path(const std::string& path) {
    path_ = path;
}

void TaskBar::set_scan(ScanState state, size_t done, size_t total) {
    state_ = state;
    done_ = done;
    total_ = total;
}

void TaskBar::set_totals(size_t entries, uint64_t bytes) {
    entries_ = entries;
    bytes_ = bytes;
}

namespace {

int seg_width(const char* label, const std::string& value) {
    // Shape: + LABEL value +   => 1+1+len(label)+1+len(value)+1+1
    return 5 + static_cast<int>(std::strlen(label)) + static_cast<int>(value.size());
}

// Draw one bordered segment: + LABEL value +   (returns next free column).
int draw_segment(int y, int x, const char* label, const std::string& value, int value_cp) {
    render_colored(y, x, "+ ", theme_pair(ThemeColor::Border));
    x += 2;
    const std::string lbl(label);
    render_colored(y, x, lbl + " ", theme_pair(ThemeColor::Title));
    x += static_cast<int>(lbl.size()) + 1;
    render_colored(y, x, value, value_cp);
    x += static_cast<int>(value.size());
    if (x < screen_width()) {
        render_colored(y, x, " +", theme_pair(ThemeColor::Border));
        x += 2;
    }
    return x + 1; // one-column gap between segments
}

std::string clock_string() {
    const std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return buf;
}

} // namespace

void TaskBar::render() {
    if (y_ < 0) return;
    const int h = screen_height();
    const int w = screen_width();
    if (w < 36 || y_ >= h) return;

    // Clock pinned to the far right.
    const std::string clock = clock_string();
    const int clock_x = w - static_cast<int>(clock.size());
    const int limit = clock_x - 2; // keep a small breathing gap before the clock

    // SCAN segment value + color (the live background-scan indicator).
    std::string scan_value;
    int scan_cp;
    if (state_ == ScanState::Running) {
        if (total_ > 0) {
            const int barw = 8;
            int filled = static_cast<int>((done_ * static_cast<size_t>(barw)) / total_);
            if (filled > barw) filled = barw;
            std::string bar(static_cast<size_t>(barw), '-');
            for (int i = 0; i < filled; ++i) bar[static_cast<size_t>(i)] = '#';
            char buf[64];
            std::snprintf(buf, sizeof(buf), "RUNNING %zu/%zu [%s]", done_, total_, bar.c_str());
            scan_value = buf;
        } else {
            scan_value = "RUNNING";
        }
        scan_cp = theme_pair(ThemeColor::Hi);
    } else {
        scan_value = "READY";
        scan_cp = theme_pair(ThemeColor::Ok);
    }

    int x = 0;

    // PATH (mandatory, value truncatable on narrow terminals).
    const int path_label_w = seg_width("PATH", "");
    const int reserve_scan = seg_width("SCAN", scan_value) + 1;
    int avail_for_path = limit - reserve_scan - x - 1;
    if (avail_for_path < 8) avail_for_path = 8;
    std::string path_text = truncate(path_, avail_for_path - path_label_w);
    x = draw_segment(y_, x, "PATH", path_text, theme_pair(ThemeColor::Font));

    // SCAN (mandatory).
    if (x + reserve_scan <= limit) {
        x = draw_segment(y_, x, "SCAN", scan_value, scan_cp);
    }

    // TOTAL and ENTRIES (optional, dropped first when space runs out).
    const std::string total_text = filesystem::Entry::format_size(bytes_);
    if (x + seg_width("TOTAL", total_text) + 1 <= limit) {
        x = draw_segment(y_, x, "TOTAL", total_text, theme_pair(ThemeColor::Font));
    }

    const std::string entries_text = std::to_string(entries_);
    if (x + seg_width("ENTRIES", entries_text) + 1 <= limit) {
        x = draw_segment(y_, x, "ENTRIES", entries_text, theme_pair(ThemeColor::Muted));
    }

    // Transient note (warn color), truncated to fit before the clock.
    if (!note_.empty()) {
        const int room = limit - x;
        if (room >= 4) {
            render_colored(y_, x, "  ", theme_pair(ThemeColor::Muted));
            render_colored(y_, x + 2, truncate(note_, room - 2), theme_pair(ThemeColor::Warn));
        }
    }

    // Clock (right-aligned).
    render_colored(y_, clock_x, clock, theme_pair(ThemeColor::Muted));
}

} // namespace tachikoma::ui
