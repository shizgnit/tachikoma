#pragma once

#include <string>
#include <vector>
#include <functional>

namespace tachikoma::ui {

/// Command for slash command system
struct Command {
    std::string name;           // e.g. "help"
    std::string description;    // e.g. "Show available commands"
    std::function<void()> action; // callback to execute
};

/// Slash command input bar (bottom of screen)
class CommandBar {
public:
    CommandBar();
    ~CommandBar();

    /// Register a command
    void register_command(const std::string& name, const std::string& description, std::function<void()> action);

    /// Set viewport position
    void set_viewport(int y, int x, int width);

    /// Render the command bar
    void render();

    /// Handle key input - returns true if command bar consumed the key
    bool handle_input(int key);

    /// Check if command bar is active (user is typing)
    bool is_active() const;

    /// Get the current input text
    const std::string& input() const;

    /// Get the command list for completion
    const std::vector<Command>& commands() const;

    /// Show help overlay (dismissible with any key)
    void show_help();

    /// Check if help overlay is showing
    bool is_help_shown() const;

private:
    /// Find matching commands for tab completion
    std::vector<std::string> find_matches(const std::string& prefix) const;

    /// Render help overlay
    void render_help_overlay();

    int y_{0};
    int x_{0};
    int width_{0};

    std::string input_;
    bool active_{false};
    std::vector<Command> commands_;
    std::vector<std::string> suggestions_;
    int suggestion_index_{-1};

    bool help_mode_{false};
};

} // namespace tachikoma::ui
