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
    // A bare scanning path is auxiliary context; only a message or an active
    // progress bar should claim the status row (otherwise a stale path would
    // pin it forever and hide live updates).
    return !message_.empty() || has_progress_;
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

    // Live progress bar leads (it is the indicator), followed by whatever text
    // explains it. The message wins over the scanning path so live updates are
    // never hidden behind a stale value.
    if (has_progress_) {
        std::string tail = !message_.empty() ? message_ : scanning_path_;
        text = progress_bar();
        if (!tail.empty()) {
            text += " ";
            text += tail;
        }
        color = 3; // cyan while a scan is in flight
    } else if (!message_.empty()) {
        text = message_;
    }

    // Truncate from the LEFT so the meaningful tail (path end / counts) stays visible.
    std::string out = text;
    if (width_ > 4 && static_cast<int>(out.length()) > width_) {
        int drop = static_cast<int>(out.length()) - width_ + 4; // leave room for "..."
        if (drop < 0) drop = 0;
        out = "..." + out.substr(drop);
    }

    render_colored(y_, x_, truncate(out, width_), color);
}

} // namespace tachikoma::ui
