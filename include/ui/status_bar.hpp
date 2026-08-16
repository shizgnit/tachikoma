#pragma once

#include <string>

namespace tachikoma::ui {

/// Status bar for showing progress and working status (top of screen)
class StatusBar {
public:
    StatusBar();
    ~StatusBar();

    /// Set viewport position
    void set_viewport(int y, int x, int width);

    /// Set a status message
    void set_message(const std::string& message);

    /// Set the current path being scanned (shown during active scan)
    void set_scanning_path(const std::string& path);

    /// Set progress (0.0 to 1.0)
    void set_progress(double progress);

    /// Clear status and progress
    void clear();

    /// Render the status bar
    void render();

    /// Check if there's a status message or progress to show
    bool has_content() const;

private:
    int y_{0};
    int x_{0};
    int width_{0};

    std::string message_;
    std::string scanning_path_;
    double progress_{-1.0}; // -1 means no progress
    bool has_progress_{false};
};

} // namespace tachikoma::ui
