#include "ui/terminal.hpp"
#include "ui/renderer.hpp"
#include "ui/input.hpp"
#include "ui/tree_view.hpp"
#include "ui/logo.hpp"
#include "ui/command_bar.hpp"
#include "ui/task_bar.hpp"
#include "ui/theme.hpp"
#include "filesystem/scanner.hpp"
#include "filesystem/entry.hpp"
#include "filesystem/size_estimator.hpp"
#include "concurrency/task_tracker.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <algorithm>
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
    // CLI flags (handled before curses init so they work non-interactively)
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--version" || arg == "-v") {
            std::cout << "tachikoma " TACHIKOMA_VERSION "\n";
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout <<
                "tachikoma - terminal-based filesystem reconnaissance\n"
                "\n"
                "Usage: tachikoma [path] [--version | --help]\n"
                "\n"
                "Scans the given directory tree (default: current directory)\n"
                "and shows an interactive, size-aware listing. Press h inside\n"
                "the UI for key bindings; q quits.\n";
            return 0;
        }
    }

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
        ui::TaskBar taskbar;
        ui::CommandBar command_bar;
        filesystem::Scanner scanner;

        // Background task infrastructure
        concurrency::TaskTracker tracker(4);
        filesystem::SizeEstimator estimator(tracker);

        int term_w = terminal.width();
        int term_h = terminal.height();

        // btop-style layout (full-width frame):
        //   row 0       : box top border with the [ DIRECTORIES ] title block
        //   rows 1..h-4 : directory tree — the primary display area
        //   row h-3     : box bottom border
        //   row h-2     : task bar (path, live scan state, totals, clock)
        //   row h-1     : command input line (text entry stays exactly as before)
        int content_h = std::max(1, term_h - 4);

        taskbar.set_viewport(term_h - 2);
        command_bar.set_viewport(term_h - 1, 1, term_w - 2);

        // Frame borders: a full btop-style box around the directory panel —
        // top edge with title block, bottom edge, and vertical side rails.
        // The tree viewport (x=1..w-2) sits exactly inside the rails.
        auto draw_frame = [&](int w, int h) {
            if (w < 8 || h < 6) return;
            const int bc = ui::theme_pair(ui::ThemeColor::Border);
            ui::draw_box_top(0, 0, w, bc);
            ui::render_bold(0, 1, "[ DIRECTORIES ]", ui::theme_pair(ui::ThemeColor::Title));
            const int side_h = h - 4; // rows between top edge (0) and bottom edge (h-3)
            ui::draw_box_side(1, 0, side_h, bc);
            ui::draw_box_side(1, w - 1, side_h, bc);
            ui::draw_box_bottom(h - 3, 0, w, bc);
        };

        // Shared app state
        AppState app_state;

        // Scan-state bookkeeping for the task bar: one latched "done" flag per
        // scan (so the completion note appears exactly once), a settled flag so
        // the bar stays READY after the note fades, and transient notes with a TTL.
        bool scan_latched_done{false};
        bool taskbar_settled{true};
        std::string active_note;
        std::chrono::steady_clock::time_point note_set_at{};
        const auto NOTE_TTL = std::chrono::seconds(4);

        // Command input + main-loop state (declared early so commands can flag redraws).
        bool running = true;
        bool redraw_needed = true; // force initial draw

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

        /// Helper: start background size estimation for top-level entries.
        /// Waits (with live UI refreshes) for any in-flight work from a previous
        /// scan, then resets the tracker so progress counters and "N folders
        /// estimated" stay accurate across /scan and F5 rescans.
        auto start_size_estimation = [&]() {
            while (tracker.total_tasks() > 0 && !tracker.all_done()) {
                tracker.drain_results(); // apply nothing; just drain stale payloads
                terminal.refresh();      // keep the visible state from freezing
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            tracker.reset();

            {
                std::lock_guard<std::mutex> lock(app_state.mtx);
                app_state.sizes_ready = false;
            }

            size_t num_tasks = estimator.submit_directory_tasks(app_state.top_level_entries);
            scan_latched_done = false; // re-arm the completion note for this scan
            taskbar_settled   = false;
            active_note.clear();
            taskbar.set_scan(ui::TaskBar::ScanState::Running, 0, num_tasks);
        };

        /// Helper: do initial scan and start estimation (results are drained in the main loop)
        auto do_full_scan = [&]() {
            // Immediate first frame: framed view with the task bar before the
            // shallow listing starts, so feedback is never missing.
            taskbar.set_path(target_path);
            scan_latched_done = false;
            taskbar_settled   = true; // show READY while the shallow list runs
            active_note.clear();
            terminal.clear();
            draw_frame(term_w, term_h);
            taskbar.render();
            command_bar.render();
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
            // TreeView::set_viewport takes (y_start, x_start, height, width).
            tree_view.set_viewport(1, 1, content_h, term_w - 2);
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

        // Transient task-bar note (auto-expires after NOTE_TTL).
        auto show_note = [&](const std::string& text) {
            active_note = text;
            note_set_at = std::chrono::steady_clock::now();
            redraw_needed = true;
        };

        command_bar.register_command("path", "Show current scan path", [&]() {
            show_note("Current path: " + target_path);
        });

        command_bar.register_command("status", "Show system status", [&]() {
            show_note("Tachikoma v0.1.0 - All systems operational");
        });

        // Initial scan (shallow list + background size estimation with live progress)
        do_full_scan();

        // Main loop
        std::time_t last_clock_sec{-1};
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
                if (tracker.all_done()) {
                    app_state.sizes_ready = true;
                }
                redraw_needed = true;
            }

            // Keep the task-bar scan state live and accurate on every iteration.
            {
                const size_t total = tracker.total_tasks();
                const size_t done  = std::min<size_t>(tracker.completed_tasks(), total);

                if (total > 0 && done < total) {
                    // In progress: live done/total with a mini bar in the task bar.
                    scan_latched_done = false;
                    taskbar_settled   = false;
                    taskbar.set_scan(ui::TaskBar::ScanState::Running, done, total);
                } else if (total > 0) {
                    // All size tasks finished: latch completion once, show the note
                    // briefly, then settle back to READY.
                    const auto now = std::chrono::steady_clock::now();
                    if (!scan_latched_done) {
                        scan_latched_done = true;
                        active_note  = "Scan complete: " + std::to_string(total) + " folders estimated";
                        note_set_at  = now;
                        taskbar_settled = false;
                        redraw_needed   = true;
                    } else if (!taskbar_settled && now - note_set_at > NOTE_TTL) {
                        taskbar_settled = true; // stays READY until the next scan
                        active_note.clear();
                        redraw_needed  = true;
                    }
                    taskbar.set_scan(taskbar_settled
                        ? ui::TaskBar::ScanState::Idle
                        : ui::TaskBar::ScanState::Running);
                } else {
                    // No tasks at all (e.g. a path with no subdirectories): READY.
                    scan_latched_done = false;
                    taskbar_settled   = true;
                    taskbar.set_scan(ui::TaskBar::ScanState::Idle);
                }

                // Expire transient notes (/path, /status).
                if (!active_note.empty() && std::chrono::steady_clock::now() - note_set_at > NOTE_TTL) {
                    active_note.clear();
                    redraw_needed = true;
                }
                taskbar.set_note(active_note);
            }

            // Advance the task-bar clock once per second.
            const std::time_t now_sec = std::time(nullptr);
            if (now_sec != last_clock_sec) {
                last_clock_sec  = now_sec;
                redraw_needed   = true;
            }

            // Only redraw when something actually changed
            if (redraw_needed) {
                terminal.clear();

                // Frame borders with the [ DIRECTORIES ] title block.
                draw_frame(terminal.width(), terminal.height());

                // Update tree view children from latest scan results (without resetting selection)
                {
                    std::lock_guard<std::mutex> lock(app_state.mtx);
                    tree_view.update_children(app_state.top_level_entries);
                }
                tree_view.render();

                // Task bar: live totals for the scanned entries, then render.
                size_t entries = 0;
                uint64_t bytes = 0;
                {
                    std::lock_guard<std::mutex> lock(app_state.mtx);
                    entries = app_state.top_level_entries.size();
                    for (const auto& e : app_state.top_level_entries) {
                        bytes += (e.type == filesystem::Entry::Type::Directory) ? e.total_size : e.size;
                    }
                }
                taskbar.set_totals(entries, bytes);
                taskbar.render();

                // Command bar (text input line).
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
