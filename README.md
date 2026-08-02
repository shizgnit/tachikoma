# Tachikoma

> *"Memories, I can't believe they can delete that!"*

A cross-platform TUI (Text User Interface) filesystem reconnaissance tool inspired by Ghost in the Shell.

## Features

- Interactive filesystem tree view with size totals
- Color-coded file types and directories
- Keyboard navigation (vim-style: j/k/h/l)
- Animated startup sequence
- Cross-platform support (macOS, Linux, Windows)

## Building

### Prerequisites

- CMake 3.24+
- C++20 compatible compiler
- [vcpkg](https://vcpkg.io/) for dependency management

### Build Steps

```bash
# Bootstrap vcpkg if not already installed
./vcpkg/bootstrap-vcpkg.sh  # macOS/Linux
# or
.\vcpkg\bootstrap-vcpkg.bat  # Windows

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Run
./build/tachikoma /path/to/scan
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

| Key | Action |
|-----|--------|
| ↑/j | Move up |
| ↓/k | Move down |
| →/l/Enter | Expand directory |
| ←/h | Collapse directory |
| F5 | Refresh scan |
| q | Quit |

## Architecture

```
tachikoma/
├── src/
│   ├── app/          # Main application
│   ├── ui/           # TUI library (terminal, rendering, input, tree view)
│   └── filesystem/   # Filesystem scanning library
├── include/
│   ├── ui/           # TUI headers
│   └── filesystem/   # Filesystem headers
└── CMakeLists.txt
```

## License

MIT
