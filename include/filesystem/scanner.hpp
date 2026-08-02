#pragma once

#include "filesystem/entry.hpp"
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <atomic>

namespace tachikoma::filesystem {

/// Callback type for scan progress updates
using ProgressCallback = std::function<void(uint64_t files_scanned, uint64_t total_size)>;

/// Scans the filesystem and builds a tree of entries
class Scanner {
public:
    Scanner();
    ~Scanner();

    /// Scan a directory and return the root entry
    Entry scan(const std::string& path, int max_depth = 3);

    /// Scan with progress callback
    Entry scan(const std::string& path, ProgressCallback on_progress, int max_depth = 3);

    /// Get the total size of a path
    static uint64_t get_total_size(const std::string& path);

    /// Get a list of top-level entries in a directory
    static std::vector<Entry> list_directory(const std::string& path);

    /// Load children of a directory on demand (lazy loading)
    void load_children(Entry& entry);

    /// Check if scanning should stop (for cancellation)
    void cancel();

private:
    void scan_recursive(Entry& entry, int depth, int max_depth,
                       ProgressCallback on_progress);

    std::atomic<bool> cancelled_{false};
};

} // namespace tachikoma::filesystem
