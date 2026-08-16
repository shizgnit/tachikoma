#include "filesystem/scanner.hpp"
#include <filesystem>
#include <algorithm>
#include <functional>
#include <system_error>

namespace fs = std::filesystem;

namespace tachikoma::filesystem {

Scanner::Scanner() = default;
Scanner::~Scanner() = default;

namespace {

/// Recursively sum the file sizes under `path` (true du-style size).
/// If `on_file` is non-null it is invoked once per regular file with its size,
/// allowing callers to report progress for deep walks.
uint64_t walk_directory(const std::string& path, const std::function<void(uint64_t)>& on_file) {
    uint64_t total = 0;
    try {
        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(path, ec);
             !ec && it != fs::recursive_directory_iterator(); ++it) {
            if (!it->is_regular_file(ec)) continue;
            uint64_t size = it->file_size(ec);
            if (ec) break; // unreadable file: keep running total
            total += size;
            if (on_file) on_file(size);
        }
    } catch (const std::exception&) {
        // Permission errors etc.: return what we have so far.
    }
    return total;
}

/// Directories first (largest true total first), then files (largest first).
void sort_children(std::vector<Entry>& children) {
    std::sort(children.begin(), children.end(), [](const Entry& a, const Entry& b) {
        if (a.type != b.type) return a.type == Entry::Type::Directory;
        if (a.type == Entry::Type::Directory) return a.total_size > b.total_size;
        return a.size > b.size;
    });
}

} // namespace

void Scanner::note_file(uint64_t size, ProgressCallback on_progress) {
    ++scan_files_;
    scan_bytes_ += size;
    if (on_progress) {
        on_progress(scan_files_, scan_bytes_);
    }
}

void Scanner::scan_recursive(Entry& entry, int level, int max_depth,
                            ProgressCallback on_progress, PathCallback on_path) {
    // `level` is the depth of `entry` itself (root == 0). A directory's
    // children are materialized only while its level < max_depth, but every
    // listed directory always receives a TRUE recursive total_size so the
    // displayed numbers match reality regardless of lazy-loading depth.
    if (cancelled_.load() || entry.type != Entry::Type::Directory) {
        return;
    }

    if (level >= max_depth) {
        entry.total_size = walk_directory(entry.path, nullptr);
        return;
    }

    // Report current path being scanned
    if (on_path) {
        on_path(entry.path);
    }

    std::vector<Entry> children;
    uint64_t total = 0;

    try {
        std::error_code ec;
        auto dir_it = fs::directory_iterator(entry.path, ec);
        if (!ec) {
            std::vector<fs::directory_entry> entries;
            for (; !ec && dir_it != fs::directory_iterator(); ++dir_it) {
                if (cancelled_.load()) break;
                entries.push_back(*dir_it);
            }

            // Initial order: directories first, then by name.
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                std::error_code ea, eb;
                bool ad = a.is_directory(ea);
                bool bd = b.is_directory(eb);
                if (ad != bd) return ad;
                return a.path().filename() < b.path().filename();
            });

            for (auto& de : entries) {
                if (cancelled_.load()) break;

                Entry child;
                child.path = de.path().string();
                child.name = de.path().filename().string();

                std::error_code ec2;
                bool is_dir = de.is_directory(ec2);
                if (is_dir) {
                    child.type = Entry::Type::Directory;
                    int child_level = level + 1;
                    if (child_level < max_depth) {
                        // Still within budget: list its children AND size it fully.
                        scan_recursive(child, child_level, max_depth, on_progress, on_path);
                    } else {
                        // Deepest listed level: compute the true recursive size now;
                        // its own children are loaded lazily on expansion.
                        auto on_file = [this, &on_progress](uint64_t size) {
                            note_file(size, on_progress);
                        };
                        child.total_size = walk_directory(child.path, on_file);
                    }
                } else if (!ec2 && de.is_symlink(ec2)) {
                    child.type = Entry::Type::Symlink;
                    std::error_code ec3;
                    child.size = de.file_size(ec3);
                    note_file(child.size, on_progress);
                } else {
                    child.type = Entry::Type::File;
                    std::error_code ec3;
                    child.size = de.file_size(ec3);
                    if (!ec3) note_file(child.size, on_progress);
                }

                total += (child.type == Entry::Type::Directory) ? child.total_size : child.size;
                children.push_back(std::move(child));
            }
        }
    } catch (const std::exception&) {
        // Silently handle permission errors and other exceptions.
    }

    entry.total_size = total;
    sort_children(children);
    entry.children = std::move(children);
}

Entry Scanner::scan(const std::string& path, int max_depth) {
    return scan(path, nullptr, nullptr, max_depth);
}

Entry Scanner::scan(const std::string& path, ProgressCallback on_progress, int max_depth) {
    return scan(path, on_progress, nullptr, max_depth);
}

Entry Scanner::scan(const std::string& path, ProgressCallback on_progress, PathCallback on_path, int max_depth) {
    cancelled_.store(false);
    scanning_.store(true);
    scan_files_ = 0;
    scan_bytes_ = 0;

    Entry root;
    root.path = path;
    root.name = fs::path(path).filename().string();
    root.type = Entry::Type::Directory;
    root.total_size = 0;

    scan_recursive(root, 0, max_depth, on_progress, on_path);
    scanning_.store(false);

    return root;
}

uint64_t Scanner::get_total_size(const std::string& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return 0;
    }
    return walk_directory(path, nullptr);
}

std::vector<Entry> Scanner::list_directory(const std::string& path) {
    std::vector<Entry> entries;

    std::error_code ec;
    if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        return entries;
    }

    for (auto& de : fs::directory_iterator(path, ec)) {
        Entry entry;
        entry.path = de.path().string();
        entry.name = de.path().filename().string();

        if (de.is_directory(ec)) {
            entry.type = Entry::Type::Directory;
        } else if (de.is_symlink(ec)) {
            entry.type = Entry::Type::Symlink;
        } else {
            entry.type = Entry::Type::File;
            entry.size = de.file_size(ec);
        }

        entries.push_back(std::move(entry));
    }

    // Sort: directories first
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.type != b.type) {
            return a.type == Entry::Type::Directory;
        }
        return a.name < b.name;
    });

    return entries;
}

void Scanner::cancel() {
    cancelled_.store(true);
}

bool Scanner::is_scanning() const {
    return scanning_.load();
}

void Scanner::load_children(Entry& entry) {
    if (entry.type != Entry::Type::Directory || !entry.children.empty()) {
        return;
    }

    std::vector<Entry> children;
    uint64_t total = 0;

    try {
        std::error_code ec;
        auto dir_entries = fs::directory_iterator(entry.path, ec);
        if (ec) {
            return;
        }

        std::vector<fs::directory_entry> entries;
        for (auto& de : dir_entries) {
            entries.push_back(de);
        }

        for (auto& de : entries) {
            Entry child;
            child.path = de.path().string();
            child.name = de.path().filename().string();

            std::error_code ec;
            if (de.is_directory(ec)) {
                child.type = Entry::Type::Directory;
                // True recursive size, so lazy-loaded rows match the scan totals.
                child.total_size = walk_directory(child.path, nullptr);
            } else if (de.is_symlink(ec)) {
                child.type = Entry::Type::Symlink;
                child.size = de.file_size(ec);
            } else {
                child.type = Entry::Type::File;
                child.size = de.file_size(ec);
            }

            total += (child.type == Entry::Type::Directory) ? child.total_size : child.size;
            children.push_back(std::move(child));
        }

    } catch (const std::exception&) {
        // Silently handle permission errors
    }

    if (!children.empty()) {
        entry.total_size = total;
        sort_children(children);
        entry.children = std::move(children);
    }
}

} // namespace tachikoma::filesystem
