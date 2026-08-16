#include "ui/status_bar.hpp"
#include "ui/renderer.hpp"
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

void StatusBar::set_scanning_path(const std::string& path) {
    scanning_path_ = path;
}

void StatusBar::set_progress(double progress) {
    has_progress_ = true;
    progress_ = progress;
}

void StatusBar::clear() {
    message_.clear();
    scanning_path_.clear();
    has_progress_ = false;
    progress_ = -1.0;
}

bool StatusBar::has_content() const {
    return !message_.empty() || !scanning_path_.empty() || has_progress_;
}

void StatusBar::render() {
    if (!has_content()) {
        return;
    }

    std::string text;
    int color = 2; // green
    int bar_width = 20;

    auto progress_bar = [&]() {
        int filled = static_cast<int>(progress_ * bar_width);
        if (filled < 0) filled = 0;
        if (filled > bar_width) filled = bar_width;
        std::string bar(bar_width, '-');
        for (int i = 0; i < filled; ++i) {
            bar[i] = '#';
        }
        return "[" + bar + "] " + std::to_string(static_cast<int>(progress_ * 100)) + "%";
    };

    if (!scanning_path_.empty()) {
        // Show scanning path with progress bar (path keeps its tail: it is the useful part)
        int max_path_len = width_ - 35; // room for "[scanning] [bar] NN% " + path
        if (max_path_len < 10) max_path_len = 10;
        std::string short_path = scanning_path_;
        if (static_cast<int>(short_path.length()) > max_path_len) {
            short_path = "..." + short_path.substr(short_path.length() - max_path_len + 3);
        }
        text = "[scanning] " + progress_bar() + " " + short_path;
        color = 3; // cyan for scanning
    } else if (!message_.empty()) {
        text = has_progress_ ? message_ + " " + progress_bar() : message_;
    } else if (has_progress_) {
        text = progress_bar();
    }

    render_colored(y_, x_, truncate(text, width_), color);
}

} // namespace tachikoma::ui
