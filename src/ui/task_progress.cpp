#include "ui/task_progress.hpp"
#include "ui/renderer.hpp"
#include <algorithm>
#include <sstream>

namespace tachikoma::ui {

TaskProgress::TaskProgress() = default;
TaskProgress::~TaskProgress() = default;

void TaskProgress::set_viewport(int y_start, int x_start, int height, int width) {
    y_start_ = y_start;
    x_start_ = x_start;
    height_ = height;
    width_ = width;
}

void TaskProgress::set_progress(size_t completed, size_t total, size_t running) {
    completed_ = completed;
    total_ = total;
    running_ = running;
    has_content_ = (total_ > 0);
}

void TaskProgress::render() {
    if (!has_content_ || width_ < 5 || height_ < 1) {
        return;
    }

    spinner_frame_++;

    // Line 1: label + spinner
    std::string line1 = label_ + " " + std::string(1, spinner_char());
    if (static_cast<int>(line1.length()) > width_) {
        line1 = line1.substr(0, width_ - 1);
    }
    render_colored(y_start_, x_start_, line1, 3); // cyan

    // Line 2: mini progress bar + stats
    if (height_ >= 2) {
        int bar_width = std::max(1, width_ - 18);
        int filled = 0;
        if (total_ > 0) {
            filled = static_cast<int>(
                (static_cast<double>(completed_) / static_cast<double>(total_)) * bar_width);
        }

        std::string bar;
        for (int i = 0; i < bar_width; ++i) {
            bar += (i < filled) ? '#' : '-';
        }

        std::ostringstream stats;
        stats << completed_ << "/" << total_ << " (" << running_ << " running)";

        std::string line2 = "[" + bar + "] " + stats.str();
        if (static_cast<int>(line2.length()) > width_) {
            line2 = line2.substr(0, width_);
        }
        render_colored(y_start_ + 1, x_start_, line2, 6); // blue
    }
}

void TaskProgress::set_label(const std::string& label) {
    label_ = label;
}

bool TaskProgress::has_content() const {
    return has_content_;
}

char TaskProgress::spinner_char() const {
    static const char spinner[] = {'|', '/', '-', '\\'};
    return spinner[std::abs(spinner_frame_ % 4)];
}

} // namespace tachikoma::ui
