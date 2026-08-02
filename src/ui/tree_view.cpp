#include "ui/tree_view.hpp"
#include "ui/renderer.hpp"
#include <algorithm>
#include <functional>

namespace tachikoma::ui {

TreeView::TreeView() = default;
TreeView::~TreeView() = default;

void TreeView::set_root(const filesystem::Entry& root) {
    root_ = root;
    selected_index_ = 0;
    scroll_offset_ = 0;
    build_visible_list();
}

void TreeView::build_visible_list() {
    visible_items_.clear();

    std::function<void(const filesystem::Entry&, int)> collect =
        [&](const filesystem::Entry& entry, int depth) {
        visible_items_.push_back(&entry);

        if (entry.expanded && !entry.children.empty()) {
            for (const auto& child : entry.children) {
                collect(child, depth + 1);
            }
        }
    };

    collect(root_, 0);
}

void TreeView::render_item(const filesystem::Entry& entry, int y, int x, int depth, bool is_selected) {
    // Draw tree connectors
    int pos = x + (depth * 2);
    if (depth > 0) {
        mvaddch(y, pos - 1, ' ');
    }

    // Entry icon
    const char* icon = nullptr;
    int color = 1; // default white

    switch (entry.type) {
        case filesystem::Entry::Type::Directory:
            icon = entry.expanded ? "📂" : "📁";
            color = 6; // blue
            break;
        case filesystem::Entry::Type::File:
            icon = "📄";
            color = 1; // white
            break;
        case filesystem::Entry::Type::Symlink:
            icon = "🔗";
            color = 7; // magenta
            break;
        default:
            icon = "?";
            break;
    }

    if (is_selected) {
        color = 8; // selection highlight
    }

    // Render the entry
    render_colored(y, pos, icon, color);
    render_colored(y, pos + 2, entry.name, color);

    // Render size
    std::string size_str = filesystem::Entry::format_size(
        entry.type == filesystem::Entry::Type::Directory ? entry.total_size : entry.size);
    int size_x = pos + 2 + entry.name.length() + 2;

    if (size_x + size_str.length() < width_ + x_start_) {
        render_colored(y, size_x, size_str, is_selected ? 8 : 1);
    }
}

void TreeView::render() {
    // Rebuild visible list in case of changes
    build_visible_list();

    if (visible_items_.empty()) {
        return;
    }

    // Adjust scroll offset to keep selected item visible
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + height_) {
        scroll_offset_ = selected_index_ - height_ + 1;
    }

    // Render visible items
    int y = y_start_;
    for (size_t i = static_cast<size_t>(scroll_offset_);
         i < visible_items_.size() && y < y_start_ + height_;
         ++i, ++y) {
        bool is_selected = (static_cast<int>(i) == selected_index_);
        render_item(*visible_items_[i], y, x_start_, 0, is_selected);
    }
}

void TreeView::handle_input(int key) {
    switch (key) {
        case KEY_UP:
            if (selected_index_ > 0) {
                --selected_index_;
            }
            break;
        case KEY_DOWN:
            if (selected_index_ < static_cast<int>(visible_items_.size()) - 1) {
                ++selected_index_;
            }
            break;
        case KEY_RIGHT:
        case KEY_ENTER:
            // Expand directory
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = const_cast<filesystem::Entry*>(visible_items_[selected_index_]);
                if (entry && entry->type == filesystem::Entry::Type::Directory && !entry->expanded) {
                    entry->expanded = true;
                    refresh();
                }
            }
            break;
        case KEY_LEFT:
            // Collapse directory
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = const_cast<filesystem::Entry*>(visible_items_[selected_index_]);
                if (entry && entry->type == filesystem::Entry::Type::Directory && entry->expanded) {
                    entry->expanded = false;
                    refresh();
                }
            }
            break;
        case 'j':
            if (selected_index_ < static_cast<int>(visible_items_.size()) - 1) {
                ++selected_index_;
            }
            break;
        case 'k':
            if (selected_index_ > 0) {
                --selected_index_;
            }
            break;
        case 'l':
        case 10: // newline (Enter)
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = const_cast<filesystem::Entry*>(visible_items_[selected_index_]);
                if (entry && entry->type == filesystem::Entry::Type::Directory && !entry->expanded) {
                    entry->expanded = true;
                    refresh();
                }
            }
            break;
        case 'h':
            if (selected_index_ >= 0 && selected_index_ < static_cast<int>(visible_items_.size())) {
                auto* entry = const_cast<filesystem::Entry*>(visible_items_[selected_index_]);
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

} // namespace tachikoma::ui
