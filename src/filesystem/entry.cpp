#include "filesystem/entry.hpp"
#include <sstream>
#include <iomanip>

namespace tachikoma::filesystem {

std::string Entry::format_size(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        ++unit_index;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

std::string Entry::display_name() const {
    if (type == Type::Directory) {
        return "[ " + name + " ]";
    }
    return name;
}

} // namespace tachikoma::filesystem
