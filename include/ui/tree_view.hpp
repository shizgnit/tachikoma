#pragma once

#include "filesystem/entry.hpp"
#include <vector>
#include <string>
#include <functional>
#include <ncurses.h>

namespace tachikoma::ui {

/// Callback for lazy-loading children of a directory
using LoadChildrenCallback = std::function<void(filesystem::Entry&)>;

/// Callback fired after a directory is expanded, so caller can submit size tasks
using DirectoryExpandedCallback = std::function<void(const std::vector<filesystem::Entry>& new_children)>;

/// Tree view for displaying filesystem entries
class TreeView {
public:
    TreeView();
    ~TreeView();

    /// Set the root entry to display (takes ownership via copy)
    void set_root(filesystem::Entry root);

    /// Update children of the root entry (preserves selection/scroll)
    void update_children(const std::vector<filesystem::Entry>& children);

    /// Get a mutable reference to the root entry (for applying size results recursively)
    filesystem::Entry& mutable_root();

    /// Set callback for lazy-loading directory children
    void set_load_children_callback(LoadChildrenCallback cb);

    /// Set callback fired after directory expansion (for size estimation)
    void set_directory_expanded_callback(DirectoryExpandedCallback cb);

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
    void render_item(const filesystem::Entry& entry, int y, int x, int depth, bool is_selected,
                     uint64_t max_size, int name_col_width, int bar_col_start, int bar_width, int size_col_start);
    void build_visible_list();

    /// Compute max size across all visible items (for inline bar scaling)
    uint64_t compute_max_size() const;

    filesystem::Entry root_;
    std::vector<filesystem::Entry*> visible_items_;
    int selected_index_{0};
    int scroll_offset_{0};

    int y_start_{0};
    int x_start_{0};
    int height_{0};
    int width_{0};

    LoadChildrenCallback load_children_cb_;
    DirectoryExpandedCallback directory_expanded_cb_;
};

} // namespace tachikoma::ui
