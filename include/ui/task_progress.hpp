#pragma once

#include <string>

namespace tachikoma::ui {

/// Renders a small task progress indicator widget in the bottom corner.
/// Shows completed/total tasks, a mini progress bar, and animated spinner.
class TaskProgress {
public:
    TaskProgress();
    ~TaskProgress();

    /// Set the viewport position (bottom-right corner area).
    void set_viewport(int y_start, int x_start, int height, int width);

    /// Update progress state.
    void set_progress(size_t completed, size_t total, size_t running);

    /// Render the progress widget.
    void render();

    /// Set a custom label (e.g. "Scanning folders...").
    void set_label(const std::string& label);

    /// Check if there is anything to show.
    bool has_content() const;

private:
    /// Get the current spinner character (cycles on each render).
    char spinner_char() const;

    int y_start_{0};
    int x_start_{0};
    int height_{0};
    int width_{0};

    size_t completed_{0};
    size_t total_{0};
    size_t running_{0};
    std::string label_{"Scanning..."};
    bool has_content_{false};
    int spinner_frame_{0};
};

} // namespace tachikoma::ui
