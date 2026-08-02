#include "ui/terminal.hpp"
#include "ui/renderer.hpp"
#include "ui/input.hpp"
#include "ui/tree_view.hpp"
#include "ui/logo.hpp"
#include "ui/command_bar.hpp"
#include "ui/status_bar.hpp"
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

        // Set up components
        ui::StatusBar status_bar;
        ui::CommandBar command_bar;
        filesystem::Scanner scanner;

        int term_w = terminal.width();
        int term_h = terminal.height();

        // Configure status bar (top row)
        status_bar.set_viewport(0, 2, term_w - 4);

        // Configure command bar (bottom row)
        command_bar.set_viewport(term_h - 1, 2, term_w - 4);

        // Flag for quit command
        std::atomic<bool> quit_requested{false};

        // Create tree view early so commands can reference it
        ui::TreeView tree_view;
        tree_view.set_load_children_callback([&scanner](filesystem::Entry& entry) {
            scanner.load_children(entry);
        });

        // Register slash commands
        command_bar.register_command("help", "Show available commands", [&]() {
            status_bar.set_message("Commands: /help, /quit, /scan, /path <dir>, /status");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            status_bar.clear();
        });

        command_bar.register_command("quit", "Exit the application", [&]() {
            quit_requested.store(true);
        });

        command_bar.register_command("scan", "Refresh filesystem scan", [&]() {
            status_bar.set_message("Scanning filesystem...");
            status_bar.set_progress(0.0);

            // Scan with progress
            uint64_t total_files = 0;
            auto root = scanner.scan(target_path, [&](uint64_t files, uint64_t size) {
                total_files = files;
                // Simulate progress (in reality, estimate total)
                double progress = std::min(0.95, static_cast<double>(files) / 1000.0);
                status_bar.set_progress(progress);
                terminal.refresh();
            }, 2);

            status_bar.set_progress(1.0);
            status_bar.set_message("Scan complete: " + std::to_string(total_files) + " files");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            status_bar.clear();

            tree_view.set_root(root);
        });

        command_bar.register_command("path", "Change scan path", [&]() {
            // For now, just show current path
            status_bar.set_message("Current path: " + target_path);
        });

        command_bar.register_command("status", "Show system status", [&]() {
            status_bar.set_message("Tachikoma v0.1.0 - All systems operational");
        });

        // Initial scan with status
        status_bar.set_message("Scanning filesystem...");
        status_bar.set_progress(0.0);
        terminal.refresh();

        auto root = scanner.scan(target_path, [&](uint64_t files, uint64_t size) {
            double progress = std::min(0.95, static_cast<double>(files) / 1000.0);
            status_bar.set_progress(progress);
            terminal.refresh();
        }, 2);

        status_bar.set_progress(1.0);
        status_bar.set_message("Scan complete");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        status_bar.clear();
        terminal.clear();

        // Main loop
        bool running = true;
        tree_view.set_root(root);

        // Set up tree viewport (between status bar and command bar)
        tree_view.set_viewport(2, 2, term_h - 4, term_w - 4);

        while (running) {
            // Clear and redraw
            terminal.clear();

            // Header
            ui::render_colored(0, 2, "+============================================================+", 2);
            ui::render_colored(1, 2, "|  TACHIKOMA - Filesystem Reconnaissance System               |", 2);

            // Status bar
            status_bar.render();

            // Selected item info
            auto* selected = tree_view.selected();
            if (selected) {
                std::string status = "Path: " + selected->path +
                                   " | Size: " + filesystem::Entry::format_size(
                                       selected->type == filesystem::Entry::Type::Directory ?
                                       selected->total_size : selected->size);
                ui::render_text(term_h - 2, 2, ui::truncate(status, term_w - 4));
            }

            // Render tree view
            tree_view.render();

            // Command bar
            command_bar.render();

            terminal.refresh();

            // Handle input
            int key = ui::wait_for_key();

            // Check if command bar consumed the key
            if (command_bar.handle_input(key)) {
                continue;
            }

            // Check for quit command
            if (quit_requested.load()) {
                running = false;
                continue;
            }

            // Direct key handling
            switch (key) {
                case 'q':
                case 'Q':
                    running = false;
                    break;
                case KEY_F(5):
                    // Refresh - re-scan
                    status_bar.set_message("Refreshing...");
                    status_bar.set_progress(0.0);
                    terminal.refresh();

                    root = scanner.scan(target_path, 2);
                    tree_view.set_root(root);

                    status_bar.clear();
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
