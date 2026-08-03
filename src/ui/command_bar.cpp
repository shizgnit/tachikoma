#include "ui/command_bar.hpp"
#include "ui/renderer.hpp"
#include <ncurses.h>
#include <algorithm>

namespace tachikoma::ui {

CommandBar::CommandBar() = default;
CommandBar::~CommandBar() = default;

void CommandBar::register_command(const std::string& name, const std::string& description, std::function<void()> action) {
    commands_.push_back({name, description, action});
}

void CommandBar::set_viewport(int y, int x, int width) {
    y_ = y;
    x_ = x;
    width_ = width;
}

void CommandBar::render() {
    if (help_mode_) {
        // Render help overlay
        render_help_overlay();
    } else if (active_) {
        // Show input with suggestions
        std::string prompt = "/" + input_;
        render_colored(y_, x_, prompt, 3); // cyan

        // Show suggestions if available
        if (!suggestions_.empty() && suggestion_index_ >= 0) {
            int sug_x = x_ + static_cast<int>(prompt.length()) + 1;
            if (sug_x + static_cast<int>(suggestions_[suggestion_index_].length()) < x_ + width_) {
                render_colored(y_, sug_x, "[" + suggestions_[suggestion_index_] + "]", 4); // yellow highlight
            }
        }
    } else {
        // Show hint text
        render_colored(y_, x_, "/ for commands", 1); // default white
    }
}

bool CommandBar::handle_input(int key) {
    // If help overlay is showing, dismiss on any key
    if (help_mode_) {
        help_mode_ = false;
        return true;
    }

    if (key == '/' && !active_) {
        active_ = true;
        input_.clear();
        suggestions_.clear();
        suggestion_index_ = -1;
        return true;
    }

    if (!active_) {
        return false;
    }

    // Escape to close
    if (key == 27) { // ESC
        active_ = false;
        input_.clear();
        suggestions_.clear();
        suggestion_index_ = -1;
        return true;
    }

    // Enter to execute
    if (key == 10 || key == KEY_ENTER) {
        if (!input_.empty()) {
            // Check if we have a suggestion selected
            std::string cmd = (suggestion_index_ >= 0 && !suggestions_.empty()) ?
                suggestions_[suggestion_index_] : input_;

            // Execute command
            for (const auto& command : commands_) {
                if (command.name == cmd) {
                    if (command.action) {
                        command.action();
                    }
                    active_ = false;
                    input_.clear();
                    suggestions_.clear();
                    suggestion_index_ = -1;
                    return true;
                }
            }
        } else {
            // Empty enter - just close
            active_ = false;
            input_.clear();
            suggestions_.clear();
            suggestion_index_ = -1;
            return true;
        }
    }

    // Tab for completion
    if (key == 9) { // TAB
        if (input_.empty()) {
            // Show all commands
            suggestions_.clear();
            for (const auto& cmd : commands_) {
                suggestions_.push_back(cmd.name);
            }
            suggestion_index_ = 0;
        } else if (!suggestions_.empty()) {
            // Cycle through suggestions
            suggestion_index_ = (suggestion_index_ + 1) % static_cast<int>(suggestions_.size());
        } else {
            // Find matches
            suggestions_ = find_matches(input_);
            if (!suggestions_.empty()) {
                suggestion_index_ = 0;
            }
        }
        return true;
    }

    // Backspace
    if (key == 127 || key == KEY_BACKSPACE || key == 8) {
        if (!input_.empty()) {
            input_.pop_back();
            // Update suggestions
            if (!input_.empty()) {
                suggestions_ = find_matches(input_);
                suggestion_index_ = 0;
            } else {
                suggestions_.clear();
                suggestion_index_ = -1;
            }
        }
        return true;
    }

    // Regular character input
    if (key >= 32 && key < 127) {
        input_.push_back(static_cast<char>(key));
        // Update suggestions
        suggestions_ = find_matches(input_);
        suggestion_index_ = 0;
        return true;
    }

    return false;
}

bool CommandBar::is_active() const {
    return active_;
}

const std::string& CommandBar::input() const {
    return input_;
}

const std::vector<Command>& CommandBar::commands() const {
    return commands_;
}

std::vector<std::string> CommandBar::find_matches(const std::string& prefix) const {
    std::vector<std::string> matches;
    for (const auto& cmd : commands_) {
        if (cmd.name.length() >= prefix.length() &&
            cmd.name.substr(0, prefix.length()) == prefix) {
            matches.push_back(cmd.name);
        }
    }
    return matches;
}

void CommandBar::show_help() {
    help_mode_ = true;
}

bool CommandBar::is_help_shown() const {
    return help_mode_;
}

void CommandBar::render_help_overlay() {
    int term_height = getmaxy(stdscr);
    int term_width = getmaxx(stdscr);

    // Calculate overlay size
    int overlay_height = static_cast<int>(commands_.size()) + 4; // title + commands + padding
    if (overlay_height > term_height - 4) {
        overlay_height = term_height - 4;
    }

    int start_y = (term_height - overlay_height) / 2;
    int start_x = (term_width - 60) / 2;
    if (start_x < 2) start_x = 2;

    // Title
    std::string title = "  TACHIKOMA - Available Commands  ";
    int title_x = (term_width - static_cast<int>(title.length())) / 2;
    render_colored(start_y, title_x, title, 2); // green

    // Commands
    int cmd_y = start_y + 2;
    size_t max_cmds = static_cast<size_t>(overlay_height - 4);
    for (size_t i = 0; i < commands_.size() && i < max_cmds; ++i) {
        std::string line = "  /" + commands_[i].name + " - " + commands_[i].description;
        if (static_cast<int>(line.length()) > term_width - 4) {
            line = line.substr(0, term_width - 7) + "...";
        }
        render_colored(cmd_y, 2, line, 3); // cyan for command name
        ++cmd_y;
    }

    // Footer
    std::string footer = "  Press any key to dismiss  ";
    int footer_x = (term_width - static_cast<int>(footer.length())) / 2;
    render_colored(cmd_y + 1, footer_x, footer, 4); // yellow
}

} // namespace tachikoma::ui
