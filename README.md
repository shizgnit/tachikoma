# Tachikoma

> *"Memories, I can't believe they can delete that!"*

A cross-platform TUI (Text User Interface) filesystem reconnaissance tool inspired by Ghost in the Shell. Interactively browse your filesystem with size totals, color-coded entries, and vim-style navigation.

## Features

- **Interactive filesystem tree view** with lazy-loading directory expansion
- **Slash command system** (`/help`, `/quit`, `/scan`, `/path`, `/status`) with tab completion
- **Progress bar** for long-running tasks (scanning, etc.)
- **Status bar** showing real-time task feedback
- **Color-coded file types** (directories, files, symlinks)
- **Vim-style keyboard navigation** (j/k/h/l)
- **Animated startup sequence** with Ghost in the Shell theme
- **Cross-platform support** (macOS, Linux, Windows)
- **Full test coverage** with GoogleTest (37 tests)

## Installation

### macOS / Linux — Homebrew

```bash
brew install shizgnit/tap/tachikoma
```

(The formula builds from source against the latest tagged release.)

### Windows — winget

```bash
winget install shizgnit.tachikoma
```

(Subject to review/approval in [microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs). Until it lands, use a GitHub Release zip below.)

### Any platform — GitHub Releases

Download the latest release asset for your OS from <https://github.com/shizgnit/tachikoma/releases>, unzip (Windows), and run `tachikoma` in any terminal.

## Building

### Prerequisites

- CMake 3.24+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 19.30+)
- [vcpkg](https://vcpkg.io/) for dependency management (git submodule)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/shizgnit/tachikoma.git
cd tachikoma

# Bootstrap vcpkg (macOS/Linux)
./vcpkg/bootstrap-vcpkg.sh -disableMetrics
# Windows: .\vcpkg\bootstrap-vcpkg.bat -disableMetrics

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Run
./build/tachikoma /path/to/scan
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Usage

```bash
# Scan current directory
./build/tachikoma

# Scan specific path
./build/tachikoma /Users

# Scan home directory
./build/tachikoma ~
```

## Controls

### Navigation

| Key | Action |
|-----|--------|
| `j` / `↓` | Move down |
| `k` / `↑` | Move up |
| `l` / `→` / `Enter` | Expand directory (lazy-loads children) |
| `h` / `←` | Collapse directory |
| `F5` | Refresh filesystem scan |
| `q` | Quit |

### Slash Commands

Press `/` to open the command bar, then type a command and press `Enter`:

| Command | Description |
|---------|-------------|
| `/help` | Show available commands (overlay) |
| `/quit` | Exit the application |
| `/scan` | Refresh filesystem scan with progress |
| `/path` | Show current scan path |
| `/status` | Show system status |

**Tab completion:** Press `Tab` to cycle through matching command suggestions. Press `ESC` to cancel.

## Architecture

```
tachikoma/
├── src/
│   ├── app/              # Main application entry point
│   │   └── main.cpp
│   ├── ui/               # TUI library
│   │   ├── terminal.cpp  # Terminal RAII manager (init/shutdown)
│   │   ├── renderer.cpp  # Text rendering (color, attributes, boxes)
│   │   ├── input.cpp     # Keyboard input handling
│   │   ├── tree_view.cpp # Interactive tree view component
│   │   ├── logo.cpp      # Startup screen with ASCII art
│   │   ├── command_bar.cpp  # Slash command input bar
│   │   └── status_bar.cpp   # Progress/status display
│   └── filesystem/       # Filesystem scanning library
│       ├── scanner.cpp   # Recursive directory scanner with lazy loading
│       └── entry.cpp     # Filesystem entry struct and utilities
├── include/
│   ├── ui/               # TUI headers
│   └── filesystem/       # Filesystem headers
├── tests/
│   ├── test_filesystem.cpp  # Scanner and entry tests (17 tests)
│   └── test_utils.cpp       # Size formatting and utility tests (20 tests)
├── CMakeLists.txt        # Build configuration
└── vcpkg.json           # vcpkg manifest (ncurses dependency)
```

### Design Principles

- **Modular architecture** — Filesystem and TUI are separate libraries for potential reuse
- **Lazy loading** — Directories load children on-demand (no deep pre-scanning)
- **RAII terminal management** — Clean shutdown via `TerminalGuard`
- **ASCII-safe rendering** — No UTF-8 box-drawing chars for ncurses compatibility

## License

MIT
