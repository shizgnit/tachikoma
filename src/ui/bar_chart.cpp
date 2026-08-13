#include "ui/bar_chart.hpp"
#include "ui/renderer.hpp"
#include "filesystem/size_estimator.hpp"
#include <algorithm>
#include <ncurses.h>
#include <sstream>
#include <iomanip>

namespace tachikoma::ui {

BarChart::BarChart() = default;
BarChart::~BarChart() = default;

void BarChart::set_viewport(int y_start, int x_start, int height, int width) {
    y_start_ = y_start;
    x_start_ = x_start;
    height_ = height;
    width_ = width;
}

void BarChart::update_entries(const std::vector<filesystem::Entry>& entries) {
    entries_ = entries;
    if (sort_by_size_) {
        // Sort: directories first, then by size descending
        std::sort(entries_.begin(), entries_.end(), [](const filesystem::Entry& a, const filesystem::Entry& b) {
            if (a.type != b.type) {
                return a.type == filesystem::Entry::Type::Directory;
            }
            uint64_t a_size = (a.type == filesystem::Entry::Type::Directory) ? a.total_size : a.size;
            uint64_t b_size = (b.type == filesystem::Entry::Type::Directory) ? b.total_size : b.size;
            if (a_size != b_size) {
                return a_size > b_size;
            }
            return a.name < b.name;
        });
    }
}

void BarChart::render() {
    if (entries_.empty() || width_ < 10 || height_ < 1) {
        return;
    }

    // Find max size for scaling
    uint64_t max_size = 0;
    for (const auto& e : entries_) {
        uint64_t s = (e.type == filesystem::Entry::Type::Directory) ? e.total_size : e.size;
        if (s > max_size) max_size = s;
    }
    if (max_size == 0) max_size = 1; // avoid div by zero

    // Determine how many bars to show
    int available_rows = height_ - 1; // reserve 1 row for title
    int num_bars = available_rows;
    if (max_bars_ > 0 && max_bars_ < num_bars) {
        num_bars = max_bars_;
    }
    num_bars = std::min(num_bars, static_cast<int>(entries_.size()));

    // Title row
    std::string title = " Size Breakdown (largest first)";
    render_colored(y_start_, x_start_, title, 2); // green

    // Layout: name (16ch) + " " + bar + " " + size
    int name_width = 16;
    int bar_max_width = width_ - name_width - 12; // room for name + size + gaps
    if (bar_max_width < 4) bar_max_width = 4;

    int row = y_start_ + 1;
    for (int i = 0; i < num_bars && (row < y_start_ + height_); ++i) {
        render_bar(row, entries_[i], max_size, bar_max_width);
        ++row;
    }
}

void BarChart::render_bar(int row, const filesystem::Entry& entry,
                          uint64_t max_size, int bar_width) {
    uint64_t size = (entry.type == filesystem::Entry::Type::Directory) ? entry.total_size : entry.size;

    // Name column (truncated)
    std::string name = entry.name;
    int name_w = std::min(static_cast<int>(name.length()), 15);
    std::string display_name = name.substr(0, name_w);
    if (name.length() > 15) {
        display_name = display_name.substr(0, 12) + "...";
    }

    // Prefix with icon
    char icon = '?';
    int color = 1;
    switch (entry.type) {
        case filesystem::Entry::Type::Directory:
            icon = 'D';
            color = 6; // blue
            break;
        case filesystem::Entry::Type::File:
            icon = 'F';
            color = 1; // white
            break;
        case filesystem::Entry::Type::Symlink:
            icon = '@';
            color = 7; // magenta
            break;
        default:
            break;
    }

    std::string label;
    if (entry.type == filesystem::Entry::Type::Directory) {
        label = "[" + std::string(1, icon) + "] " + display_name;
    } else {
        label = "[ " + std::string(1, icon) + "] " + display_name;
    }

    // Pad label to name_width
    while (static_cast<int>(label.length()) < 16) {
        label += ' ';
    }
    label = label.substr(0, 16);

    render_colored(row, x_start_, label, color);

    // Bar
    int bar_x = x_start_ + 17;
    int actual_bar_width = std::min(bar_width, width_ - 17 - 12);
    if (actual_bar_width < 1) actual_bar_width = 1;

    double fraction = 0.0;
    if (max_size > 0) {
        fraction = static_cast<double>(size) / static_cast<double>(max_size);
    }

    // If size is 0 and still scanning, show a pulsing dot
    if (size == 0 && entry.type == filesystem::Entry::Type::Directory) {
        render_colored(row, bar_x, "...", 4); // yellow - still scanning
    } else {
        // Build bar string
        std::string bar;
        int filled = static_cast<int>(fraction * actual_bar_width);
        for (int i = 0; i < actual_bar_width; ++i) {
            if (i < filled) {
                // Gradient: use different chars for density
                if (fraction >= 0.8) bar += '#';
                else if (fraction >= 0.5) bar += '=';
                else if (fraction >= 0.2) bar += '-';
                else bar += '.';
            } else {
                bar += ' ';
            }
        }
        render_colored(row, bar_x, bar, (fraction > 0.5) ? 3 : 4); // cyan or yellow
    }

    // Size label
    std::string size_str = filesystem::Entry::format_size(size);
    int size_x = bar_x + actual_bar_width + 1;
    if (size_x + static_cast<int>(size_str.length()) < x_start_ + width_) {
        render_colored(row, size_x, size_str, 1);
    }
}

void BarChart::set_sort_by_size(bool sort) {
    sort_by_size_ = sort;
}

void BarChart::set_max_bars(int max) {
    max_bars_ = max;
}

} // namespace tachikoma::ui
