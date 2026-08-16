#include "ui/terminal.hpp"
#include "ui/renderer.hpp"
#include "ui/input.hpp"
#include "ui/tree_view.hpp"
#include "ui/logo.hpp"
#include "ui/command_bar.hpp"
#include "ui/status_bar.hpp"
#include "ui/task_progress.hpp"
#include "filesystem/scanner.hpp"
#include "filesystem/entry.hpp"
#include "filesystem/size_estimator.hpp"
#include "concurrency/task_tracker.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>

using namespace tachikoma;

/// Shared state protected by a mutex for UI updates from main thread only.
struct AppState {
    std::vector<filesystem::Entry> top_level_entries;
    bool sizes_ready{false};
    std::mutex mtx;
};

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
        ui::TaskProgress task_progress;
        filesystem::Scanner scanner;

        // Background task infrastructure
        concurrency::TaskTracker tracker(4);
        filesystem::SizeEstimator estimator(tracker);

        int term_w = terminal.width();
        int term_h = terminal.height();

        // Status/scan-status content lives on row 1 (row 0 is the border line)
        status_bar.set_viewport(1, 2, term_w - 4);

        // Configure command bar (bottom row)
        command_bar.set_viewport(term_h - 1, 2, term_w - 4);

        // Unified layout: full-width tree with inline size bars
        //                 bottom: task progress + info row + command bar
        int pane_width  = term_w - 4;
        int pane_height = term_h - 5; // reserve header(2) + progress(1) + info(1) + cmd(1)
        if (pane_height < 4) pane_height = 4;

        task_progress.set_viewport(term_h - 3, 2, 1, pane_width);

        // Unified top-of-screen frame: full-width border on row 0, and either the
        // title or live scan status (message/progress) on row 1. Every code path
        // that shows scanning state goes through this one function so the
        // indicator can never be forgotten by a particular call site.
        auto draw_frame_top = [&](ui::StatusBar& sb) {
            int width = terminal.width();
            if (width < 8) return;
            terminal.clear();
            std::string line(width - 2, '=');
            ui::render_colored(0, 0, "+" + line + "+", 2); // green border
            if (sb.has_content()) {
                sb.render(); // row 1: scan progress / message
            } else {
                ui::render_text(1, 2, "TACHIKOMA - Filesystem Reconnaissance System");
            }
        };

        // Shared app state
        AppState app_state;

        // Flag for quit command
        std::atomic<bool> quit_requested{false};

        // Create tree view early so commands can reference it
        ui::TreeView tree_view;
        tree_view.set_load_children_callback([&scanner](filesystem::Entry& entry) {
            scanner.load_children(entry);
        });
        tree_view.set_directory_expanded_callback([&estimator](const std::vector<filesystem::Entry>& children) {
            // Submit size estimation for any child directories
            estimator.submit_directory_tasks(children);
        });

        /// Helper: start background size estimation for top-level entries
        auto start_size_estimation = [&]() {
            // Clear tracker state by creating fresh one would be ideal,
            // but we reuse. Just drain old results first.
            tracker.drain_results();

            {
                std::lock_guard<std::mutex> lock(app_state.mtx);
                app_state.sizes_ready = false;
            }

            size_t num_tasks = estimator.submit_directory_tasks(app_state.top_level_entries);
            task_progress.set_progress(0, num_tasks, 0);
            task_progress.set_label("Estimating folder sizes...");
            status_bar.set_message("Launched " + std::to_string(num_tasks) + " size estimation tasks");
        };

        /// Helper: do initial scan and start estimation (results are drained in the main loop)
        auto do_full_scan = [&]() {
            // Immediate first frame: border + scan status on row 1 before the
            // shallow listing starts, so feedback is never missing.
            status_bar.set_scanning_path(target_path);
            status_bar.set_message("Scanning filesystem...");
            status_bar.set_progress(0.0);
            draw_frame_top(status_bar);
            terminal.refresh();

            // Quick shallow scan (list top-level only)
            auto entries = filesystem::Scanner::list_directory(target_path);

            {
                std::lock_guard<std::mutex> lock(app_state.mtx);
                app_state.top_level_entries = std::move(entries);
                app_state.sizes_ready = false;
            }

            // Start background size estimation for each directory
            start_size_estimation();

            // Set tree view root
            filesystem::Entry root;
            root.path = target_path;
            root.name = std::filesystem::path(target_path).filename().string();
            if (root.name.empty()) root.name = target_path;
            root.type = filesystem::Entry::Type::Directory;
            root.expanded = true;

            {
                std::lock_guard<std::mutex> lock(app_state.mtx);
                root.children = app_state.top_level_entries;
            }
            tree_view.set_root(root);
            tree_view.set_viewport(2, 2, pane_height, pane_width);
        };

        // Register slash commands
        command_bar.register_command("help", "Show available commands", [&]() {
            command_bar.show_help();
        });

        command_bar.register_command("quit", "Exit the application", [&]() {
            quit_requested.store(true);
        });

        command_bar.register_command("scan", "Refresh filesystem scan", [&]() {
            do_full_scan();
        });

        command_bar.register_command("path", "Change scan path", [&]() {
            status_bar.set_message("Current path: " + target_path);
        });

        command_bar.register_command("status", "Show system status", [&]() {
            status_bar.set_message("Tachikoma v0.1.0 - All systems operational");
        });

        // Initial scan (shallow list + background size estimation with live progress)
        do_full_scan();

        // Main loop
        bool running = true;
        bool redraw_needed = true; // force initial draw

        while (running) {
            // Process any completed background results
            auto results = tracker.drain_results();
            if (!results.empty()) {
                {
                    std::lock_guard<std::mutex> lock(app_state.mtx);
                    estimator.apply_results(results, app_state.top_level_entries);
                    filesystem::SizeEstimator::sort_by_size(app_state.top_level_entries);
                    estimator.apply_results_recursive(results, tree_view.mutable_root());
                    filesystem::SizeEstimator::sort_by_size_recursive(tree_view.mutable_root());
                }
                task_progress.set_progress(
                    tracker.completed_tasks(), tracker.total_tasks(), tracker.running_tasks());
                if (tracker.all_done()) {
                    app_state.sizes_ready = true;
                    task_progress.set_label("Size estimation complete!");
                    status_bar.set_message(
                        "Scan complete: " +
                        std::to_string(tracker.completed_tasks()) + " folders estimated");
                }
                redraw_needed = true;
            }

            // Only redraw when something actually changed
            if (redraw_needed) {
                // Top rows: full-width border + (title or live scan status) — single code path
                draw_frame_top(status_bar);

                // Update tree view children from latest scan results (without resetting selection)
                {
                    std::lock_guard<std::mutex> lock(app_state.mtx);
                    tree_view.update_children(app_state.top_level_entries);
                }
                tree_view.render();

                // Task progress widget (bottom)
                task_progress.render();

                // Selected item info row
                auto* selected = tree_view.selected();
                if (selected) {
                    std::string status = "Path: " + selected->path +
                                        " | Size: " + filesystem::Entry::format_size(
                                           selected->type == filesystem::Entry::Type::Directory ?
                                           selected->total_size : selected->size);
                    ui::render_text(term_h - 2, 2, ui::truncate(status, term_w - 4));
                }

                // Command bar
                command_bar.render();

                terminal.refresh();
                redraw_needed = false;
            }

            // Handle input with timeout so we can poll for background results
            auto key_opt = ui::get_key_with_timeout(200);

            if (key_opt.has_value()) {
                int key = key_opt.value();

                // Check if command bar consumed the key
                if (command_bar.handle_input(key)) {
                    redraw_needed = true;
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
                        do_full_scan();
                        redraw_needed = true;
                        break;
                    default:
                        tree_view.handle_input(key);
                        redraw_needed = true;
                        break;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    terminal.shutdown();
    return 0;
}
