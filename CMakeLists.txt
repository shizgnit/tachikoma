cmake_minimum_required(VERSION 3.24)

project(tachikoma VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# Concurrency / background task library
add_library(tachikoma_concurrency STATIC
    src/concurrency/task_tracker.cpp
)
target_include_directories(tachikoma_concurrency PUBLIC include)
target_compile_features(tachikoma_concurrency PUBLIC cxx_std_20)
target_link_libraries(tachikoma_concurrency PUBLIC Threads::Threads)

# Filesystem scanning library
add_library(tachikoma_fs STATIC
    src/filesystem/scanner.cpp
    src/filesystem/entry.cpp
    src/filesystem/size_estimator.cpp
)
target_include_directories(tachikoma_fs PUBLIC include)
target_compile_features(tachikoma_fs PUBLIC cxx_std_20)
target_link_libraries(tachikoma_fs PUBLIC tachikoma_concurrency)

# TUI library
add_library(tachikoma_ui STATIC
    src/ui/terminal.cpp
    src/ui/renderer.cpp
    src/ui/input.cpp
    src/ui/tree_view.cpp
    src/ui/logo.cpp
    src/ui/command_bar.cpp
    src/ui/status_bar.cpp
    src/ui/bar_chart.cpp
    src/ui/task_progress.cpp
)
target_include_directories(tachikoma_ui PUBLIC include)
target_link_libraries(tachikoma_ui PUBLIC tachikoma_fs)
target_compile_features(tachikoma_ui PUBLIC cxx_std_20)

# Link terminal library (ncurses on Unix, PDCurses on Windows) and threads
if(WIN32)
    find_package(unofficial-pdcurses REQUIRED)
    target_link_libraries(tachikoma_ui PUBLIC unofficial::pdcurses::pdcurses)
    target_compile_definitions(tachikoma_ui PUBLIC USE_PDCURSES)
else()
    find_package(Curses REQUIRED)
    target_link_libraries(tachikoma_ui PUBLIC ${CURSES_LIBRARIES})
    target_include_directories(tachikoma_ui PUBLIC ${CURSES_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)
target_link_libraries(tachikoma_ui PUBLIC Threads::Threads)

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
