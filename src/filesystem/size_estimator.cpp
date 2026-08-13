#include "filesystem/size_estimator.hpp"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace tachikoma::filesystem {

SizeEstimator::SizeEstimator(concurrency::TaskTracker& tracker)
    : tracker_(tracker) {}

size_t SizeEstimator::submit_directory_tasks(const std::vector<Entry>& entries) {
    // Build path-to-entry map so results can be applied later
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        path_to_entry_.clear();
    }

    size_t submitted = 0;
    for (const auto& entry : entries) {
        if (entry.type == Entry::Type::Directory) {
            // Store pointer for result application
            {
                std::lock_guard<std::mutex> lock(map_mutex_);
                // We need a mutable copy; entries will be updated in place
                path_to_entry_[entry.path] = const_cast<Entry*>(&entry);
            }
            submit_directory(entry.path);
            ++submitted;
        }
    }
    return submitted;
}

concurrency::TaskId SizeEstimator::submit_directory(const std::string& path) {
    return tracker_.submit(path, [path]() -> uint64_t {
        return compute_total_size(path);
    });
}

void SizeEstimator::apply_results(
    const std::vector<concurrency::TaskResult>& results,
    std::vector<Entry>& entries)
{
    for (const auto& result : results) {
        if (result.state != concurrency::TaskState::Done) {
            continue;
        }

        // Find the entry with matching path
        for (auto& entry : entries) {
            if (entry.path == result.label && entry.type == Entry::Type::Directory) {
                entry.total_size = result.payload;
                break;
            }
        }
    }
}

void SizeEstimator::apply_results_recursive(
    const std::vector<concurrency::TaskResult>& results,
    Entry& root)
{
    for (const auto& result : results) {
        if (result.state != concurrency::TaskState::Done) {
            continue;
        }

        // Recursively search for matching path
        std::function<Entry*(Entry&)> find_entry = [&](Entry& node) -> Entry* {
            if (node.path == result.label && node.type == Entry::Type::Directory) {
                return &node;
            }
            for (auto& child : node.children) {
                if (Entry* found = find_entry(child)) return found;
            }
            return nullptr;
        };

        if (Entry* entry = find_entry(root)) {
            entry->total_size = result.payload;
        }
    }
}

uint64_t SizeEstimator::compute_total_size(const std::string& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return 0;
    }

    uint64_t total = 0;
    try {
        for (auto& dir_entry : fs::recursive_directory_iterator(path, ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (dir_entry.is_regular_file(ec)) {
                total += dir_entry.file_size(ec);
            }
        }
    } catch (const std::exception&) {
        // Return partial result
    }
    return total;
}

void SizeEstimator::sort_by_size(std::vector<Entry>& entries) {
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        // Directories first
        if (a.type != b.type) {
            return a.type == Entry::Type::Directory;
        }
        // Then by total_size (or size for files) descending
        uint64_t a_size = (a.type == Entry::Type::Directory) ? a.total_size : a.size;
        uint64_t b_size = (b.type == Entry::Type::Directory) ? b.total_size : b.size;
        if (a_size != b_size) {
            return a_size > b_size;
        }
        // Stable fallback: alphabetical
        return a.name < b.name;
    });
}

void SizeEstimator::sort_by_size_recursive(Entry& node) {
    sort_by_size(node.children);
    for (auto& child : node.children) {
        if (child.type == Entry::Type::Directory) {
            sort_by_size_recursive(child);
        }
    }
}

} // namespace tachikoma::filesystem
