#pragma once

#include "filesystem/entry.hpp"
#include <string>
#include <vector>

namespace tachikoma::ui {

/// Renders horizontal bar charts for folder/file sizes.
/// Entries are sorted largest-to-smallest by default.
/// Supports live updates: call update_entries() to re-render with new data.
class BarChart {
public:
    BarChart();
    ~BarChart();

    /// Set the viewport within the terminal.
    void set_viewport(int y_start, int x_start, int height, int width);

    /// Update the entries to display. Bars are re-sorted largest-to-smallest.
    void update_entries(const std::vector<filesystem::Entry>& entries);

    /// Render the bar chart at the configured viewport.
    void render();

    /// Set whether to sort by size (default true).
    void set_sort_by_size(bool sort);

    /// Set the maximum number of bars to show (default: fit in viewport).
    void set_max_bars(int max);

private:
    /// Render a single horizontal bar.
    void render_bar(int row, const filesystem::Entry& entry, uint64_t max_size, int bar_width);

    /// Choose a bar character based on fill fraction.
    char bar_char(double fraction) const;

    int y_start_{0};
    int x_start_{0};
    int height_{0};
    int width_{0};

    std::vector<filesystem::Entry> entries_;
    bool sort_by_size_{true};
    int max_bars_{0};  // 0 = unlimited (fit in viewport)
};

} // namespace tachikoma::ui
