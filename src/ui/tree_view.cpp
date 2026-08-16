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
    // Show the top-level entries by default (ncdu-style first screen).
    if (root_.type == filesystem::Entry::Type::Directory) {
        root_.expanded = true;
    }
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
        visible_items_.push_back({&entry, depth});

        if (entry.expanded && !entry.children.empty()) {
            for (auto& child : entry.children) {
                collect(child, depth + 1);
            }
        }
    };

    collect(root_, 0);
}

namespace {

constexpr int INDENT_STEP = 2; // horizontal space per tree level
constexpr int ICON_W = 2;      // icon column (">", "+", "@") plus gap to name
constexpr int SIZE_FIELD = 8;  // minimum width of the right-aligned size text field
constexpr int MIN_BAR = 6;     // minimum width reserved for the size bar

} // namespace

uint64_t TreeView::compute_max_size() const {
    uint64_t max_size = 0;
    for (const auto& vi : visible_items_) {
        if (!vi.entry) continue;
        uint64_t s = (vi.entry->type == filesystem::Entry::Type::Directory)
            ? vi.entry->total_size : vi.entry->size;
        if (s > max_size) max_size = s;
    }
    return max_size;
}

void TreeView::render_item(const VisibleItem& item, int y, bool is_selected, const Layout& layout) {
    auto& entry = *item.entry;
    int depth = item.depth;
    int pos = x_start_ + (depth * INDENT_STEP);

    // Draw tree indentation
    for (int d = 0; d < depth; ++d) {
        render_char(y, x_start_ + (d * INDENT_STEP), '|');
    }

    // Icon and color
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

    // Render icon and name (name truncated so it never collides with the size column)
    render_colored(y, pos, icon, color);

    int name_x = pos + ICON_W;
    int name_budget = layout.size_x - name_x - 1; // leave a gap before the size field
    if (name_budget < 1) {
        return; // no room for anything but the icon in this window width
    }
    std::string name = entry.name;
    if (static_cast<int>(name.length()) > name_budget) {
        name = truncate(name, name_budget);
    }
    render_colored(y, name_x, name, color);

    // Size value: directories show their true recursive total.
    uint64_t size_val = (entry.type == filesystem::Entry::Type::Directory)
        ? entry.total_size : entry.size;

    if (layout.size_x >= x_start_ + layout.size_field) {
        // Right-align the size text inside the shared, fixed-width field so all
        // rows' digits line up vertically.
        std::string size_str = filesystem::Entry::format_size(size_val);
        while (static_cast<int>(size_str.length()) < layout.size_field) {
            size_str = " " + size_str;
        }
        render_colored(y, layout.size_x, size_str, is_selected ? 8 : 3);

        // Size bar: filled proportionally to the largest visible entry.
        int bar_x = layout.size_x + layout.size_field;
        int bar_w = layout.right_edge - 1 - bar_x;
        if (bar_w >= MIN_BAR) {
            if (layout.max_size > 0 && size_val > 0) {
                double ratio = static_cast<double>(size_val) / static_cast<double>(layout.max_size);
                int filled = static_cast<int>(ratio * bar_w);
                if (filled > bar_w) filled = bar_w;

                std::string bar(bar_w, ' ');
                for (int i = 0; i < filled && i < bar_w; ++i) {
                    bar[i] = '#';
                }
                render_colored(y, bar_x, bar, is_selected ? 8 : 4);
            } else if (entry.type == filesystem::Entry::Type::Directory) {
                render_colored(y, bar_x, "...", 4); // size still being estimated
            }
        }
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

    // Pick ONE uniform size-column position for this frame: just past the widest
    // (indent + icon + name) row. This keeps sizes right-aligned in a single
    // vertical column regardless of entry name lengths or terminal backend.
    int right_edge = x_start_ + width_;
    Layout layout;
    layout.right_edge = right_edge;
    layout.max_size = compute_max_size();

    int longest = 0;
    for (const auto& vi : visible_items_) {
        if (!vi.entry) continue;
        int w = (vi.depth * INDENT_STEP) + ICON_W + static_cast<int>(vi.entry->name.length());
        if (w > longest) longest = w;
    }

    // Size field wide enough for the longest formatted size in this frame
    // ("719.99 MB" is 9 chars, "999.99 PB" too).
    int size_field = SIZE_FIELD;
    for (const auto& vi : visible_items_) {
        if (!vi.entry) continue;
        uint64_t s = (vi.entry->type == filesystem::Entry::Type::Directory)
            ? vi.entry->total_size : vi.entry->size;
        int l = static_cast<int>(filesystem::Entry::format_size(s).length());
        if (l > size_field) size_field = l;
    }
    layout.size_field = std::min(size_field, 12);

    int name_area = longest + 1; // widest row plus the gap before the size field
    int max_name_area = right_edge - x_start_ - layout.size_field - MIN_BAR;
    if (max_name_area < 0) max_name_area = 0;
    if (name_area > max_name_area) name_area = max_name_area;

    layout.size_x = x_start_ + name_area;

    int y = y_start_;
    for (int i = scroll_offset_;
         i < static_cast<int>(visible_items_.size()) && y < y_start_ + height_;
         ++i, ++y) {
        bool is_selected = (i == selected_index_);
        render_item(visible_items_[i], y, is_selected, layout);
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
                auto* entry = visible_items_[selected_index_].entry;
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
                auto* entry = visible_items_[selected_index_].entry;
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
        return visible_items_[selected_index_].entry;
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
