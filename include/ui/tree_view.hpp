#pragma once

#include "filesystem/entry.hpp"
#include <vector>
#include <string>
#include <functional>
#include <ncurses.h>

namespace tachikoma::ui {

/// Callback for lazy-loading children of a directory
using LoadChildrenCallback = std::function<void(filesystem::Entry&)>;

/// Tree view for displaying filesystem entries
class TreeView {
public:
    TreeView();
    ~TreeView();

    /// Set the root entry to display (takes ownership via copy)
    void set_root(filesystem::Entry root);

    /// Set callback for lazy-loading directory children
    void set_load_children_callback(LoadChildrenCallback cb);

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
    std::vector<filesystem::Entry*> visible_items_;
    int selected_index_{0};
    int scroll_offset_{0};

    int y_start_{0};
    int x_start_{0};
    int height_{0};
    int width_{0};

    LoadChildrenCallback load_children_cb_;
};

} // namespace tachikoma::ui
