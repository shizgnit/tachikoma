#include "ui/status_bar.hpp"
#include "ui/renderer.hpp"
#include <ncurses.h>
#include <string>

namespace tachikoma::ui {

StatusBar::StatusBar() = default;
StatusBar::~StatusBar() = default;

void StatusBar::set_viewport(int y, int x, int width) {
    y_ = y;
    x_ = x;
    width_ = width;
}

void StatusBar::set_message(const std::string& message) {
    message_ = message;
}

void StatusBar::set_progress(double progress) {
    has_progress_ = true;
    progress_ = progress;
}

void StatusBar::clear() {
    message_.clear();
    has_progress_ = false;
    progress_ = -1.0;
}

bool StatusBar::has_content() const {
    return !message_.empty() || has_progress_;
}

void StatusBar::render() {
    if (message_.empty() && !has_progress_) {
        return;
    }

    std::string text;
    if (!message_.empty() && has_progress_) {
        // Show message with progress bar
        int bar_width = 20;
        int filled = static_cast<int>(progress_ * bar_width);
        std::string bar;
        for (int i = 0; i < bar_width; ++i) {
            bar += (i < filled) ? "#" : "-";
        }
        text = message_ + " [" + bar + "] " + std::to_string(static_cast<int>(progress_ * 100)) + "%";
    } else if (!message_.empty()) {
        text = message_;
    } else if (has_progress_) {
        int bar_width = 20;
        int filled = static_cast<int>(progress_ * bar_width);
        std::string bar;
        for (int i = 0; i < bar_width; ++i) {
            bar += (i < filled) ? "#" : "-";
        }
        text = "[" + bar + "] " + std::to_string(static_cast<int>(progress_ * 100)) + "%";
    }

    // Truncate to fit
    if (static_cast<int>(text.length()) > width_) {
        text = text.substr(0, width_ - 3) + "...";
    }

    render_colored(y_, x_, text, 2); // green
}

} // namespace tachikoma::ui
