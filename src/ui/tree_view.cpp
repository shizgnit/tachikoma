#include "ui/tree_view.hpp"
#include "ui/renderer.hpp"
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

void TreeView::render_item(const filesystem::Entry& entry, int y, int x, int depth, bool is_selected) {
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
    render_colored(y, pos + 2, entry.name, color);

    std::string size_str = filesystem::Entry::format_size(
        entry.type == filesystem::Entry::Type::Directory ? entry.total_size : entry.size);
    int size_x = pos + 2 + static_cast<int>(entry.name.length()) + 2;

    if (size_x + static_cast<int>(size_str.length()) < width_ + x_start_) {
        render_colored(y, size_x, size_str, is_selected ? 8 : 1);
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

    int y = y_start_;
    for (int i = scroll_offset_;
         i < static_cast<int>(visible_items_.size()) && y < y_start_ + height_;
         ++i, ++y) {
        bool is_selected = (i == selected_index_);
        render_item(*visible_items_[i], y, x_start_, 0, is_selected);
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
                if (entry && entry->type == filesystem::Entry::Type::Directory && !entry->expanded) {
                    entry->expanded = true;
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

} // namespace tachikoma::ui
