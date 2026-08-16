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

/// Callback type for reporting the current path being scanned
using PathCallback = std::function<void(const std::string& current_path)>;

/// Scans the filesystem and builds a tree of entries
class Scanner {
public:
    Scanner();
    ~Scanner();

    /// Scan a directory and return the root entry
    Entry scan(const std::string& path, int max_depth = 3);

    /// Scan with progress callback
    Entry scan(const std::string& path, ProgressCallback on_progress, int max_depth = 3);

    /// Scan with progress and path callbacks
    Entry scan(const std::string& path, ProgressCallback on_progress, PathCallback on_path, int max_depth = 3);

    /// Get the total size of a path
    static uint64_t get_total_size(const std::string& path);

    /// Get a list of top-level entries in a directory
    static std::vector<Entry> list_directory(const std::string& path);

    /// Load children of a directory on demand (lazy loading)
    void load_children(Entry& entry);

    /// Check if scanning should stop (for cancellation)
    void cancel();

    /// Check if a scan is currently running
    bool is_scanning() const;

private:
    /// Count one scanned file and notify the progress callback.
    void note_file(uint64_t size, ProgressCallback on_progress);

    void scan_recursive(Entry& entry, int level, int max_depth,
                       ProgressCallback on_progress, PathCallback on_path);

    std::atomic<bool> cancelled_{false};
    std::atomic<bool> scanning_{false};

    // Cumulative counters for the current scan (drives progress callbacks).
    uint64_t scan_files_{0};
    uint64_t scan_bytes_{0};
};

} // namespace tachikoma::filesystem
