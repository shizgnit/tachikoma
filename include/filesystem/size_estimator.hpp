#pragma once

#include "filesystem/entry.hpp"
#include "concurrency/task_tracker.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace tachikoma::filesystem {

/// Computes recursive directory sizes using background tasks.
/// Each visible folder is submitted as a separate task to the TaskTracker.
/// Results flow through a lock-free queue back to the UI thread.
class SizeEstimator {
public:
    explicit SizeEstimator(concurrency::TaskTracker& tracker);

    /// Submit size estimation tasks for all directories in the given entry list.
    /// Returns the number of tasks submitted.
    size_t submit_directory_tasks(const std::vector<Entry>& entries);

    /// Submit a single directory for size estimation.
    concurrency::TaskId submit_directory(const std::string& path);

    /// Update entries with the latest size results from the tracker.
    /// Call this after draining results from the queue.
    void apply_results(const std::vector<concurrency::TaskResult>& results, std::vector<Entry>& entries);

    /// Recursively apply size results against a tree of entries (including expanded children).
    void apply_results_recursive(const std::vector<concurrency::TaskResult>& results, Entry& root);

    /// Recursively compute the total size of a directory (blocking).
    static uint64_t compute_total_size(const std::string& path);

    /// Sort entries by total_size descending (directories first, then by size).
    static void sort_by_size(std::vector<Entry>& entries);

    /// Recursively sort all children within each parent by size descending.
    static void sort_by_size_recursive(Entry& node);

private:
    concurrency::TaskTracker& tracker_;

    // Map from path to Entry pointer for applying results
    mutable std::mutex map_mutex_;
    std::unordered_map<std::string, Entry*> path_to_entry_;
};

} // namespace tachikoma::filesystem
