#include "ui/command_bar.hpp"
#include "ui/renderer.hpp"
#include "ui/theme.hpp"
#include "ui/input.hpp" // KEY_* codes (platform input abstraction)
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
        render_help_overlay();
        return;
    }

    if (!active_) {
        // Idle hint: muted text with blue command keywords (btop style).
        int x = x_;
        auto kw  = [&](const std::string& t) { render_colored(y_, x, t, theme_pair(ThemeColor::Hi));   x += static_cast<int>(t.size()); };
        auto dim = [&](const std::string& t) { render_colored(y_, x, t, theme_pair(ThemeColor::Muted)); x += static_cast<int>(t.size()); };
        kw("/help");  dim("  ");
        kw("/scan <path>");  dim("   ");
        kw("/path <dir>");  dim("   ");
        kw("/status");  dim("   ");
        kw("/quit");
        return;
    }

    // Active input: btop-style highlighted block around the typed text,
    // with a frost cursor marker and yellow suggestions.
    const std::string text = "/" + input_;
    const int len = static_cast<int>(text.size());
    const int x = x_;
    fill_row(y_, x, len + 5, theme_pair(ThemeColor::SelText)); // "[ /scan# ]"

    render_colored(y_, x,        "[",  theme_pair(ThemeColor::SelAccent));
    render_bold   (y_, x + 2,    text, theme_pair(ThemeColor::SelText));
    render_colored(y_, x + 2 + len, "#", theme_pair(ThemeColor::SelAccent));

    if (!suggestions_.empty() && suggestion_index_ >= 0) {
        const std::string& s = suggestions_[static_cast<size_t>(suggestion_index_)];
        int sug_x = x + len + 7; // past "[ /scan# ] "
        if (sug_x < x + width_) {
            std::string shown = "[" + s + "]";
            if (sug_x + static_cast<int>(shown.size()) > x + width_) {
                shown = truncate(shown, static_cast<size_t>(x + width_ - sug_x));
            }
            render_colored(y_, sug_x, shown, theme_pair(ThemeColor::Warn));
        }
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
    int term_height = screen_height();
    int term_width = screen_width();

    // Calculate overlay size
    int overlay_height = static_cast<int>(commands_.size()) + 4; // title + commands + padding
    if (overlay_height > term_height - 4) {
        overlay_height = term_height - 4;
    }

    int start_y = (term_height - overlay_height) / 2;
    int start_x = (term_width - 60) / 2;
    if (start_x < 2) start_x = 2;

    // Title (frost accent)
    std::string title = "TACHIKOMA — Available Commands";
    int title_x = (term_width - static_cast<int>(title.length())) / 2;
    render_bold(start_y, title_x, title, theme_pair(ThemeColor::Title));

    // Divider under the title
    if (term_width > 10) {
        std::string div(std::min<size_t>(56, static_cast<size_t>(term_width - 4)), '-');
        render_colored(start_y + 1, (term_width - static_cast<int>(div.size())) / 2, div, theme_pair(ThemeColor::Border));
    }

    // Commands: blue keyword + muted description (btop help-list style)
    int cmd_y = start_y + 3;
    size_t max_cmds = static_cast<size_t>(overlay_height - 5);
    for (size_t i = 0; i < commands_.size() && i < max_cmds; ++cmd_y, ++i) {
        const std::string& name = commands_[i].name;
        const std::string& desc = commands_[i].description;
        int x = std::max(2, (term_width - 40) / 2);
        render_colored(cmd_y, x, "/" + name, theme_pair(ThemeColor::Hi));
        x += static_cast<int>(name.size()) + 3;
        if (!desc.empty() && x < term_width - 2) {
            std::string d = desc;
            int room = term_width - 2 - x;
            if (static_cast<int>(d.size()) > room) d = truncate(d, static_cast<size_t>(room));
            render_colored(cmd_y, x, " — " + d, theme_pair(ThemeColor::Muted));
        }
    }

    // Footer
    std::string footer = "Press any key to dismiss";
    int footer_x = (term_width - static_cast<int>(footer.length())) / 2;
    render_colored(cmd_y + 1, footer_x, footer, theme_pair(ThemeColor::Warn));
}

} // namespace tachikoma::ui
