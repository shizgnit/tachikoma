#pragma once

#include "filesystem/entry.hpp"
#include <vector>
#include <string>
#include <ncurses.h>

namespace tachikoma::ui {

/// Tree view for displaying filesystem entries
class TreeView {
public:
    TreeView();
    ~TreeView();

    /// Set the root entry to display
    void set_root(const filesystem::Entry& root);

    /// Render the tree view
    void render();

    /// Handle key input
    void handle_input(int key);

    /// Get the currently selected entry
    const filesystem::Entry* selected() const;

    /// Get the number of visible items
    size_t visible_count() const;

    /// Set the viewport position
    void set_viewport(int y_start, int x_start, int height, int width);

    /// Refresh the view (e.g., after expanding a directory)
    void refresh();

private:
    void render_item(const filesystem::Entry& entry, int y, int x, int depth, bool is_selected);
    void build_visible_list();

    filesystem::Entry root_;
    std::vector<const filesystem::Entry*> visible_items_;
    int selected_index_{0};
    int scroll_offset_{0};

    int y_start_{0};
    int x_start_{0};
    int height_{0};
    int width_{0};
};

} // namespace tachikoma::ui
