#include "ui/logo.hpp"
#include "ui/renderer.hpp"
#include "ui/theme.hpp"
#include "ui/input.hpp"
#include <string>
#include <thread>
#include <vector>

namespace tachikoma::ui {

std::vector<std::string> get_logo() {
    return {
        "  +---------------------------+  ",
        "  |   ___                   |   ",
        "  |  /   \\                  |   ",
        "  | | T |  TACHIKOMA        |   ",
        "  |  \\___/                  |   ",
        "  +---------------------------+  ",
        "",
        "  T A C H I K O M A",
        "",
        "  Filesystem Reconnaissance System",
    };
}

std::vector<std::string> get_boot_messages() {
    return {
        "[SYSTEM] Initializing Tachikoma v0.1.0...",
        "[KERNEL] Loading cyberbrain interface...",
        "[NET] Establishing wired connection...",
        "[SCAN] Filesystem reconnaissance module loaded",
        "[UI] Terminal interface online",
        "[STATUS] All systems operational",
        "[READY] Awaiting orders, Major!",
    };
}

void render_startup_screen() {
    auto logo = get_logo();
    auto messages = get_boot_messages();

    int term_width = screen_width();
    int term_height = screen_height();

    // Clear screen
    clear_window();

    // Calculate starting position to center logo
    int logo_height = static_cast<int>(logo.size());
    int start_y = (term_height - logo_height) / 2;
    if (start_y < 2) start_y = 2;

    // Render logo with the theme palette (btop-style accents)
    for (int i = 0; i < logo_height && (start_y + i) < term_height; ++i) {
        int x = center_x(logo[i], term_width);
        if (i < 6) {
            // Tachikoma box art - nord blue accent
            render_colored(start_y + i, x, logo[i], theme_pair(ThemeColor::Hi));
        } else if (i == 7) {
            // "TACHIKOMA" title - frost bold
            render_bold(start_y + i, x, logo[i], theme_pair(ThemeColor::Title));
        } else {
            // Subtitle - muted
            render_colored(start_y + i, x, logo[i], theme_pair(ThemeColor::Muted));
        }
    }

    ::refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // Render boot messages with scrolling effect
    int msg_y = start_y + logo_height + 2;
    if (msg_y >= term_height) {
        msg_y = term_height - static_cast<int>(messages.size()) - 1;
    }

    for (const auto& msg : messages) {
        if (msg_y < term_height) {
            // Color based on prefix (theme-mapped)
            int cp = theme_pair(ThemeColor::Muted);
            if (msg.find("[SCAN]") != std::string::npos) {
                cp = theme_pair(ThemeColor::Hi);
            } else if (msg.find("[READY]") != std::string::npos) {
                cp = theme_pair(ThemeColor::Ok);
            } else if (msg.find("[ERROR]") != std::string::npos) {
                cp = theme_pair(ThemeColor::Crit);
            }

            render_colored(msg_y, 2, msg, cp);
            ::refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            ++msg_y;
        }
    }

    // Wait for key press (hint in accent blue)
    render_colored(term_height - 1, 2, "Press any key to continue...", theme_pair(ThemeColor::Hi));
    ::refresh();
    wait_for_key();
    clear_window();
}

} // namespace tachikoma::ui
