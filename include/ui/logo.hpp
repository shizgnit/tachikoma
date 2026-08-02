#pragma once

#include <string>
#include <vector>

namespace tachikoma::ui {

/// Get the ASCII art logo for the Tachikoma
std::vector<std::string> get_logo();

/// Get the boot sequence text (scrolling messages)
std::vector<std::string> get_boot_messages();

/// Render the startup screen with logo and boot messages
void render_startup_screen();

} // namespace tachikoma::ui
