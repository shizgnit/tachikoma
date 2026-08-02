include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Filesystem tests
add_executable(tachikoma_fs_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_filesystem.cpp
)
target_link_libraries(tachikoma_fs_tests PRIVATE
    tachikoma_fs
    GTest::gtest_main
)
target_compile_features(tachikoma_fs_tests PRIVATE cxx_std_20)

# Utility tests (non-ncurses parts)
add_executable(tachikoma_util_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.cpp
)
target_link_libraries(tachikoma_util_tests PRIVATE
    tachikoma_fs
    GTest::gtest_main
)
target_compile_features(tachikoma_util_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(tachikoma_fs_tests)
gtest_discover_tests(tachikoma_util_tests)
