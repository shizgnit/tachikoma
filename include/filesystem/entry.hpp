#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <optional>
#include <vector>

namespace tachikoma::filesystem {

/// Represents a single filesystem entry (file or directory)
struct Entry {
    enum class Type { File, Directory, Symlink, Unknown };

    std::string path;
    std::string name;
    Type type{Type::Unknown};
    uint64_t size{0};
    uint64_t total_size{0}; // For directories: sum of all children
    bool expanded{false}; // UI state for tree view

    // Children (only populated for expanded directories)
    std::vector<Entry> children;

    /// Format size in human-readable form (KB, MB, GB, etc.)
    static std::string format_size(uint64_t bytes);

    /// Get the display name for this entry
    std::string display_name() const;
};

} // namespace tachikoma::filesystem
