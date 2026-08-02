#include "filesystem/scanner.hpp"
#include <filesystem>
#include <algorithm>
#include <system_error>

namespace fs = std::filesystem;

namespace tachikoma::filesystem {

Scanner::Scanner() = default;
Scanner::~Scanner() = default;

void Scanner::scan_recursive(Entry& entry, int depth, int max_depth,
                            ProgressCallback on_progress) {
    if (cancelled_.load()) {
        return;
    }

    if (entry.type != Entry::Type::Directory) {
        return;
    }

    // Don't list contents if we've exceeded max depth
    if (depth >= max_depth) {
        return;
    }

    uint64_t files_scanned = 0;
    uint64_t total_size = 0;

    try {
        std::error_code ec;
        auto dir_entries = fs::directory_iterator(entry.path, ec);
        if (ec) {
            return;
        }

        std::vector<fs::directory_entry> entries;
        for (auto& de : dir_entries) {
            if (cancelled_.load()) break;
            entries.push_back(de);
        }

        // Sort: directories first, then by name
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            std::error_code ec_a, ec_b;
            bool a_dir = a.is_directory(ec_a);
            bool b_dir = b.is_directory(ec_b);
            if (a_dir != b_dir) return a_dir;
            return a.path().filename().string() < b.path().filename().string();
        });

        for (auto& de : entries) {
            if (cancelled_.load()) break;

            Entry child;
            child.path = de.path().string();
            child.name = de.path().filename().string();

            std::error_code ec;
            if (de.is_directory(ec)) {
                child.type = Entry::Type::Directory;
                child.size = 0;
            } else if (de.is_symlink(ec)) {
                child.type = Entry::Type::Symlink;
                child.size = de.file_size(ec);
            } else {
                child.type = Entry::Type::File;
                child.size = de.file_size(ec);
            }

            entry.children.push_back(std::move(child));
            files_scanned++;
            total_size += entry.children.back().size;

            if (on_progress) {
                on_progress(files_scanned, total_size);
            }
        }

        // Recurse into directories if we have depth left
        if (depth < max_depth) {
            for (auto& child : entry.children) {
                if (cancelled_.load()) break;
                if (child.type == Entry::Type::Directory) {
                    scan_recursive(child, depth + 1, max_depth, on_progress);
                }
            }
        }

        // Calculate total size for directories (from children)
        for (const auto& child : entry.children) {
            if (child.type == Entry::Type::Directory && child.total_size > 0) {
                entry.total_size += child.total_size;
            } else {
                entry.total_size += child.size;
            }
        }

    } catch (const std::exception&) {
        // Silently handle permission errors and other exceptions
    }
}

Entry Scanner::scan(const std::string& path, int max_depth) {
    return scan(path, nullptr, max_depth);
}

Entry Scanner::scan(const std::string& path, ProgressCallback on_progress, int max_depth) {
    cancelled_.store(false);

    Entry root;
    root.path = path;
    root.name = fs::path(path).filename().string();
    root.type = Entry::Type::Directory;
    root.total_size = 0;

    scan_recursive(root, 0, max_depth, on_progress);

    return root;
}

uint64_t Scanner::get_total_size(const std::string& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return 0;
    }

    uint64_t total = 0;
    try {
        for (auto& entry : fs::recursive_directory_iterator(path, ec)) {
            if (entry.is_regular_file(ec)) {
                total += entry.file_size(ec);
            }
        }
    } catch (const std::exception&) {
        // Return what we have so far
    }
    return total;
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

void Scanner::load_children(Entry& entry) {
    if (entry.type != Entry::Type::Directory || !entry.children.empty()) {
        return;
    }

    uint64_t total_size = 0;

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

        // Sort: directories first, then by name
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            std::error_code ec_a, ec_b;
            bool a_dir = a.is_directory(ec_a);
            bool b_dir = b.is_directory(ec_b);
            if (a_dir != b_dir) return a_dir;
            return a.path().filename().string() < b.path().filename().string();
        });

        for (auto& de : entries) {
            Entry child;
            child.path = de.path().string();
            child.name = de.path().filename().string();

            std::error_code ec;
            if (de.is_directory(ec)) {
                child.type = Entry::Type::Directory;
                child.size = 0;
            } else if (de.is_symlink(ec)) {
                child.type = Entry::Type::Symlink;
                child.size = de.file_size(ec);
            } else {
                child.type = Entry::Type::File;
                child.size = de.file_size(ec);
            }

            entry.children.push_back(std::move(child));
            total_size += entry.children.back().size;
        }

        entry.total_size += total_size;

    } catch (const std::exception&) {
        // Silently handle permission errors
    }
}

} // namespace tachikoma::filesystem
