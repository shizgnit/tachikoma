#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tachikoma::ui {

/// btop-style bottom task bar: a row of small bordered segments showing the
/// scan path, live background-scan state, size totals and the clock.
class TaskBar {
public:
    enum class ScanState { Idle, Running };

    /// Set the row to render on (full width is used automatically)
    void set_viewport(int y);

    void set_path(const std::string& path);

    /// Live background-scan state for the SCAN segment.
    void set_scan(ScanState state, size_t done = 0, size_t total = 0);

    void set_totals(size_t entries, uint64_t bytes);

    /// Transient note (e.g. "scan complete: N folders estimated"), warn color.
    void set_note(const std::string& note) { note_ = note; }
    void clear_note() { note_.clear(); }

    void render();

private:
    int y_{-1};
    std::string path_;
    ScanState state_{ScanState::Idle};
    size_t done_{0};
    size_t total_{0};
    size_t entries_{0};
    uint64_t bytes_{0};
    std::string note_;
};

} // namespace tachikoma::ui
