#include "ui/logo.hpp"
#include "ui/renderer.hpp"
#include "ui/terminal.hpp"
#include "ui/input.hpp"
#include <thread>
#include <chrono>
#include <ncurses.h>

namespace tachikoma::ui {

std::vector<std::string> get_logo() {
    return {
        "  ██████╗██╗   ██╗██████╗ ███████╗██████╗ ",
        " ██╔════╝╚██╗ ██╔╝██╔══██╗██╔════╝██╔══██╗",
        " ██║      ╚████╔╝ ██████╔╝█████╗  ██████╔╝",
        " ██║       ╚██╔╝  ██╔══██╗██╔══╝  ██╔══██╗",
        " ╚██████╗   ██║   ██████╔╝███████╗██║  ██║",
        "  ╚═════╝   ╚═╝   ╚═════╝ ╚══════╝╚═╝  ╚═╝",
        "",
        "  ╔╦╗  ╔═╗  ╔╦╗  ╔╦╗  ╔╦╗  ╔═╗  ╔╦╗  ╔═╗ ",
        "  ║║   ║ ║   ║   ║║   ║║   ║ ║   ║   ║ ║ ",
        "  ╩ ╩  ╚═╝   ╩   ╩   ╩   ╚═╝   ╩   ╚═╝ ",
        "",
        "        T A C H I K O M A",
        "   Filesystem Reconnaissance System",
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

    int term_width = getmaxx(stdscr);
    int term_height = getmaxy(stdscr);

    // Clear screen
    clear();

    // Calculate starting position to center logo
    int logo_height = static_cast<int>(logo.size());
    int start_y = (term_height - logo_height) / 2;
    if (start_y < 2) start_y = 2;

    // Render logo with color
    for (int i = 0; i < logo_height && (start_y + i) < term_height; ++i) {
        int x = center_x(logo[i], term_width);
        if (i < 6) {
            // Main "TACHIKOMA" text - green
            render_colored(start_y + i, x, logo[i], 2);
        } else if (i >= 7 && i <= 9) {
            // Subtitle area - cyan
            render_colored(start_y + i, x, logo[i], 3);
        } else {
            // Bottom text - yellow
            render_colored(start_y + i, x, logo[i], 4);
        }
    }

    refresh();
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // Render boot messages with scrolling effect
    int msg_y = start_y + logo_height + 2;
    if (msg_y >= term_height) {
        msg_y = term_height - static_cast<int>(messages.size()) - 1;
    }

    for (const auto& msg : messages) {
        if (msg_y < term_height) {
            // Color based on prefix
            int cp = 1; // default
            if (msg.find("[SCAN]") != std::string::npos) {
                cp = 6; // blue
            } else if (msg.find("[READY]") != std::string::npos) {
                cp = 2; // green
            } else if (msg.find("[ERROR]") != std::string::npos) {
                cp = 5; // red
            }

            attron(COLOR_PAIR(cp));
            mvaddstr(msg_y, 2, msg.c_str());
            attroff(COLOR_PAIR(cp));
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            ++msg_y;
        }
    }

    // Wait for key press
    mvaddstr(term_height - 1, 2, "Press any key to continue...");
    refresh();
    wait_for_key();
    clear();
}

} // namespace tachikoma::ui
