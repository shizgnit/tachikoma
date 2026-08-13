#include "ui/tree_view.hpp"
#include "ui/renderer.hpp"
#include "filesystem/size_estimator.hpp"
#include <algorithm>
#include <functional>

namespace tachikoma::ui {

TreeView::TreeView() = default;
TreeView::~TreeView() = default;

void TreeView::set_root(filesystem::Entry root) {
    root_ = std::move(root);
    selected_index_ = 0;
    scroll_offset_ = 0;
    build_visible_list();
}

void TreeView::update_children(const std::vector<filesystem::Entry>& children) {
    // Merge: preserve expanded state and loaded children for entries that
    // already exist, and append new entries from the updated list.
    for (const auto& new_child : children) {
        bool found = false;
        for (auto& existing : root_.children) {
            if (existing.path == new_child.path) {
                // Update size fields but keep expanded + loaded children
                existing.total_size = new_child.total_size;
                existing.size = new_child.size;
                existing.type = new_child.type;
                found = true;
                break;
            }
        }
        if (!found) {
            root_.children.push_back(new_child);
        }
    }
    // Resort siblings by size after merge
    filesystem::SizeEstimator::sort_by_size(root_.children);
    build_visible_list();
}

void TreeView::build_visible_list() {
    visible_items_.clear();

    std::function<void(filesystem::Entry&, int)> collect =
        [&](filesystem::Entry& entry, int depth) {
        visible_items_.push_back(&entry);

        if (entry.expanded && !entry.children.empty()) {
            for (auto& child : entry.children) {
                collect(child, depth + 1);
            }
        }
    };

    collect(root_, 0);
}

uint64_t TreeView::compute_max_size() const {
    uint64_t max_size = 0;
    for (auto* item : visible_items_) {
        if (item) {
            uint64_t s = (item->type == filesystem::Entry::Type::Directory) ? item->total_size : item->size;
            if (s > max_size) max_size = s;
        }
    }
    return max_size;
}

void TreeView::render_item(const filesystem::Entry& entry, int y, int x, int depth, bool is_selected,
                           uint64_t max_size, int name_col_width, int bar_col_start, int bar_width, int size_col_start) {
    int pos = x + (depth * 2);
    if (depth > 0) {
        mvaddch(y, pos - 1, ' ');
    }

    const char* icon = nullptr;
    int color = 1;

    switch (entry.type) {
        case filesystem::Entry::Type::Directory:
            icon = entry.expanded ? "+" : ">";
            color = 6;
            break;
        case filesystem::Entry::Type::File:
            icon = " ";
            color = 1;
            break;
        case filesystem::Entry::Type::Symlink:
            icon = "@";
            color = 7;
            break;
        default:
            icon = "?";
            break;
    }

    if (is_selected) {
        color = 8;
    }

    render_colored(y, pos, icon, color);

    // Truncate name to fit fixed-width name column
    std::string display_name = entry.name;
    if (static_cast<int>(display_name.length()) > name_col_width) {
        display_name = display_name.substr(0, name_col_width - 3) + "...";
    }
    render_colored(y, pos + 2, display_name, color);

    uint64_t size = (entry.type == filesystem::Entry::Type::Directory) ? entry.total_size : entry.size;

    // Inline horizontal bar at fixed column position
    if (max_size > 0 && size > 0) {
        double fraction = static_cast<double>(size) / static_cast<double>(max_size);
        int filled = static_cast<int>(fraction * bar_width);

        char bar_char = '.';
        if (fraction >= 0.8) bar_char = '#';
        else if (fraction >= 0.5) bar_char = '=';
        else if (fraction >= 0.2) bar_char = '-';

        std::string bar;
        for (int i = 0; i < bar_width; ++i) {
            bar += (i < filled) ? bar_char : ' ';
        }
        render_colored(y, bar_col_start, bar, (fraction > 0.5) ? 3 : 4);
    } else if (size == 0 && entry.type == filesystem::Entry::Type::Directory) {
        render_colored(y, bar_col_start, "...", 4); // still scanning
    }

    // Size label right-aligned at fixed column
    std::string size_str = filesystem::Entry::format_size(size);
    if (size_col_start + static_cast<int>(size_str.length()) < width_ + x_start_) {
        render_colored(y, size_col_start, size_str, is_selected ? 8 : 1);
    }
}

void TreeView::render() {
    build_visible_list();

    if (visible_items_.empty()) {
        return;
    }

    // Clamp indices
    if (selected_index_ < 0) selected_index_ = 0;
    if (selected_index_ >= static_cast<int>(visible_items_.size()))
        selected_index_ = static_cast<int>(visible_items_.size()) - 1;

    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + height_) {
        scroll_offset_ = selected_index_ - height_ + 1;
    }

    if (scroll_offset_ < 0) scroll_offset_ = 0;

    // Compute max size for inline bar scaling
    uint64_t max_size = compute_max_size();
    if (max_size == 0) max_size = 1;

    // Fixed column layout: icon(2) + name_col + " " + bar_col + " " + size_col
    // Size column is always 10 chars, name column is capped so bar gets room
    int size_col_width = 10;
    int name_col_width = std::min(width_ - 2 - size_col_width - 6, 30); // cap name at 30
    if (name_col_width < 8) name_col_width = 8;

    int bar_col_start = x_start_ + 2 + name_col_width + 1;
    int size_col_start = x_start_ + width_ - size_col_width;
    int bar_width = size_col_start - bar_col_start - 1;
    if (bar_width < 2) bar_width = 2;

    int y = y_start_;
    for (int i = scroll_offset_;
         i < static_cast<int>(visible_items_.size()) && y < y_start_ + height_;
         ++i, ++y) {
        bool is_selected = (i == selected_index_);
        render_item(*visible_items_[i], y, x_start_, 0, is_selected, max_size,
                    name_col_width, bar_col_start, bar_width, size_col_start);
    }
}

void TreeView::handle_input(int key) {
    switch (key) {
        case KEY_UP:
        case 'k':
            if (selected_index_ > 0) --selected_index_;
            break;
        case KEY_DOWN:
        case 'j':
            if (selected_index_ < static_cast<int>(visible_items_.size()) - 1) ++selected_index_;
            break;
        case KEY_RIGHT:
        case KEY_ENTER:
        case 'l':
        case 10:
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = visible_items_[selected_index_];
                if (entry && entry->type == filesystem::Entry::Type::Directory) {
                    // Toggle expand/collapse
                    entry->expanded = !entry->expanded;
                    if (entry->expanded) {
                        // Lazy-load children if not already loaded
                        if (load_children_cb_ && entry->children.empty()) {
                            load_children_cb_(*entry);
                        }
                        // Notify caller so size tasks can be submitted for new children
                        if (directory_expanded_cb_) {
                            directory_expanded_cb_(entry->children);
                        }
                    }
                    refresh();
                }
            }
            break;
        case KEY_LEFT:
        case 'h':
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = visible_items_[selected_index_];
                if (entry && entry->type == filesystem::Entry::Type::Directory && entry->expanded) {
                    entry->expanded = false;
                    refresh();
                }
            }
            break;
    }
}

const filesystem::Entry* TreeView::selected() const {
    if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
        return visible_items_[selected_index_];
    }
    return nullptr;
}

size_t TreeView::visible_count() const {
    return visible_items_.size();
}

void TreeView::set_viewport(int y_start, int x_start, int height, int width) {
    y_start_ = y_start;
    x_start_ = x_start;
    height_ = height;
    width_ = width;
}

void TreeView::refresh() {
    build_visible_list();
}

filesystem::Entry& TreeView::mutable_root() {
    return root_;
}

void TreeView::set_load_children_callback(LoadChildrenCallback cb) {
    load_children_cb_ = std::move(cb);
}

void TreeView::set_directory_expanded_callback(DirectoryExpandedCallback cb) {
    directory_expanded_cb_ = std::move(cb);
}

} // namespace tachikoma::ui
