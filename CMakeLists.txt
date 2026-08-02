cmake_minimum_required(VERSION 3.24)

project(tachikoma VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# Filesystem scanning library
add_library(tachikoma_fs STATIC
    src/filesystem/scanner.cpp
    src/filesystem/entry.cpp
)
target_include_directories(tachikoma_fs PUBLIC include)
target_compile_features(tachikoma_fs PUBLIC cxx_std_20)

# TUI library
add_library(tachikoma_ui STATIC
    src/ui/terminal.cpp
    src/ui/renderer.cpp
    src/ui/input.cpp
    src/ui/tree_view.cpp
    src/ui/logo.cpp
)
target_include_directories(tachikoma_ui PUBLIC include)
target_link_libraries(tachikoma_ui PUBLIC tachikoma_fs)
target_compile_features(tachikoma_ui PUBLIC cxx_std_20)

# Link ncurses
find_package(Curses REQUIRED)
target_link_libraries(tachikoma_ui PUBLIC ${CURSES_LIBRARIES})
target_include_directories(tachikoma_ui PUBLIC ${CURSES_INCLUDE_DIRS})

# Main application
add_executable(tachikoma src/app/main.cpp)
target_link_libraries(tachikoma PRIVATE tachikoma_ui tachikoma_fs)
target_compile_features(tachikoma PRIVATE cxx_std_20)

# Testing
enable_testing()
add_subdirectory(tests)

# Install targets
install(TARGETS tachikoma DESTINATION bin)
install(TARGETS tachikoma_ui tachikoma_fs DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
