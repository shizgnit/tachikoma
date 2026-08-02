#include "ui/terminal.hpp"
#include "ui/renderer.hpp"
#include "ui/input.hpp"
#include "ui/tree_view.hpp"
#include "ui/logo.hpp"
#include "filesystem/scanner.hpp"
#include "filesystem/entry.hpp"

#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

using namespace tachikoma;

int main(int argc, char* argv[]) {
    // Determine target path
    std::string target_path = ".";
    if (argc > 1) {
        target_path = argv[1];
    }

    // Initialize terminal
    ui::Terminal terminal;
    terminal.init();
    terminal.set_cursor_visible(false);

    try {
        // Show startup screen
        ui::render_startup_screen();

        // Scan filesystem
        std::atomic<bool> scanning{true};
        std::string status_msg = "Scanning filesystem...";

        // Show scanning status
        int term_height = terminal.height();
        mvaddstr(term_height - 2, 2, status_msg.c_str());
        refresh();

        // Perform scan
        filesystem::Scanner scanner;
        auto root = scanner.scan(target_path, [&](uint64_t files, uint64_t size) {
            // Update progress (simplified - in production, use proper threading)
        }, 2); // max depth of 2 for initial view

        scanning.store(false);
        terminal.clear();

        // Main loop
        bool running = true;
        ui::TreeView tree_view;
        tree_view.set_root(root);

        int term_w = terminal.width();
        int term_h = terminal.height();

        // Set up viewport
        tree_view.set_viewport(3, 2, term_h - 5, term_w - 4);

        while (running) {
            // Clear and redraw
            terminal.clear();

            // Header
            ui::render_colored(0, 2, "╔══════════════════════════════════════════════════════════════╗", 2);
            ui::render_colored(1, 2, "║  TACHIKOMA - Filesystem Reconnaissance System               ║", 2);
            ui::render_colored(2, 2, "╚══════════════════════════════════════════════════════════════╝", 2);

            // Status bar
            auto* selected = tree_view.selected();
            if (selected) {
                std::string status = "Path: " + selected->path +
                                   " | Size: " + filesystem::Entry::format_size(
                                       selected->type == filesystem::Entry::Type::Directory ?
                                       selected->total_size : selected->size);
                ui::render_text(term_h - 2, 2, ui::truncate(status, term_w - 4));
            }

            // Help bar
            ui::render_text(term_h - 1, 2,
                "↑/↓: Navigate  →/Enter: Expand  ←: Collapse  q: Quit  F5: Refresh");

            // Render tree view
            tree_view.render();

            terminal.refresh();

            // Handle input
            int key = ui::wait_for_key();

            switch (key) {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case KEY_F(5):
                    // Refresh - re-scan
                    terminal.clear();
                    mvaddstr(term_h - 2, 2, "Refreshing...");
                    refresh();

                    root = scanner.scan(target_path, 2);
                    tree_view.set_root(root);
                    break;
                default:
                    tree_view.handle_input(key);
                    break;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    terminal.shutdown();
    return 0;
}
