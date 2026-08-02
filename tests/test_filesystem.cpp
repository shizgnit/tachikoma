#include <gtest/gtest.h>
#include "filesystem/entry.hpp"
#include "filesystem/scanner.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace tachikoma::filesystem;

// ============================================================
// Entry Tests
// ============================================================

TEST(EntryTest, FormatSizeBytes) {
    EXPECT_EQ(Entry::format_size(0), "0.00 B");
    EXPECT_EQ(Entry::format_size(1), "1.00 B");
    EXPECT_EQ(Entry::format_size(500), "500.00 B");
}

TEST(EntryTest, FormatSizeKilobytes) {
    EXPECT_EQ(Entry::format_size(1024), "1.00 KB");
    EXPECT_EQ(Entry::format_size(1536), "1.50 KB");
    EXPECT_EQ(Entry::format_size(10240), "10.00 KB");
}

TEST(EntryTest, FormatSizeMegabytes) {
    EXPECT_EQ(Entry::format_size(1048576), "1.00 MB");
    EXPECT_EQ(Entry::format_size(1572864), "1.50 MB");
}

TEST(EntryTest, FormatSizeGigabytes) {
    EXPECT_EQ(Entry::format_size(1073741824ULL), "1.00 GB");
    EXPECT_EQ(Entry::format_size(2147483648ULL), "2.00 GB");
}

TEST(EntryTest, FormatSizeTerabytes) {
    EXPECT_EQ(Entry::format_size(1099511627776ULL), "1.00 TB");
}

TEST(EntryTest, DisplayNameDirectory) {
    Entry e;
    e.name = "mydir";
    e.type = Entry::Type::Directory;
    EXPECT_EQ(e.display_name(), "[ mydir ]");
}

TEST(EntryTest, DisplayNameFile) {
    Entry e;
    e.name = "file.txt";
    e.type = Entry::Type::File;
    EXPECT_EQ(e.display_name(), "file.txt");
}

TEST(EntryTest, DefaultValues) {
    Entry e;
    EXPECT_EQ(e.type, Entry::Type::Unknown);
    EXPECT_EQ(e.size, 0ULL);
    EXPECT_EQ(e.total_size, 0ULL);
    EXPECT_FALSE(e.expanded);
    EXPECT_TRUE(e.children.empty());
}

// ============================================================
// Scanner Tests
// ============================================================

TEST(ScannerTest, ScanEmptyDirectory) {
    // Create a temp dir
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_empty";
    fs::create_directories(tmp);

    Scanner scanner;
    Entry root = scanner.scan(tmp.string(), 1);

    EXPECT_EQ(root.type, Entry::Type::Directory);
    EXPECT_EQ(root.name, "tachikoma_test_empty");
    EXPECT_TRUE(root.children.empty());

    fs::remove_all(tmp);
}

TEST(ScannerTest, ScanDirectoryWithFiles) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_files";
    fs::create_directories(tmp);

    // Create test files
    std::ofstream(tmp / "file1.txt") << "hello";
    std::ofstream(tmp / "file2.txt") << "world!!";  // 7 bytes
    std::ofstream(tmp / "file3.txt") << "12345678901234567890";  // 20 bytes

    Scanner scanner;
    Entry root = scanner.scan(tmp.string(), 1);

    EXPECT_EQ(root.type, Entry::Type::Directory);
    EXPECT_EQ(root.children.size(), 3u);

    // All children should be files
    for (const auto& child : root.children) {
        EXPECT_EQ(child.type, Entry::Type::File);
        EXPECT_GT(child.size, 0ULL);
    }

    // Total size should be sum of file sizes
    uint64_t expected = 5 + 7 + 20;
    EXPECT_EQ(root.total_size, expected);

    fs::remove_all(tmp);
}

TEST(ScannerTest, ScanDirectoryWithSubdirs) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_dirs";
    fs::create_directories(tmp / "subdir1" / "nested");
    fs::create_directories(tmp / "subdir2");

    std::ofstream(tmp / "root.txt") << "root file";
    std::ofstream(tmp / "subdir1" / "sub.txt") << "sub file!!";
    std::ofstream(tmp / "subdir1" / "nested" / "deep.txt") << "deep file here!!";

    Scanner scanner;
    Entry root = scanner.scan(tmp.string(), 3);

    EXPECT_EQ(root.type, Entry::Type::Directory);

    // Should have subdir1, subdir2, and root.txt
    EXPECT_GE(root.children.size(), 3u);

    // Root should have total_size > 0
    EXPECT_GT(root.total_size, 0ULL);

    fs::remove_all(tmp);
}

TEST(ScannerTest, ScanMaxDepth) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_depth";
    fs::create_directories(tmp / "a" / "b" / "c");

    std::ofstream(tmp / "top.txt") << "top";
    std::ofstream(tmp / "a" / "mid.txt") << "mid";
    std::ofstream(tmp / "a" / "b" / "c" / "deep.txt") << "deep";

    Scanner scanner;
    Entry root = scanner.scan(tmp.string(), 1);

    // At depth 1, "a" should exist but its children should not be scanned
    bool found_a = false;
    for (const auto& child : root.children) {
        if (child.name == "a" && child.type == Entry::Type::Directory) {
            found_a = true;
            // With max_depth=1, children of 'a' should not be scanned
            EXPECT_TRUE(child.children.empty());
        }
    }
    EXPECT_TRUE(found_a);

    fs::remove_all(tmp);
}

TEST(ScannerTest, ListDirectory) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_list";
    fs::create_directories(tmp);
    fs::create_directories(tmp / "adir");

    std::ofstream(tmp / "z_file.txt") << "z";
    std::ofstream(tmp / "a_file.txt") << "a";

    auto entries = Scanner::list_directory(tmp.string());

    EXPECT_FALSE(entries.empty());

    // Directories should come first
    bool found_dir = false;
    bool found_file = false;
    for (const auto& e : entries) {
        if (e.type == Entry::Type::Directory) {
            found_dir = true;
        }
        if (e.type == Entry::Type::File) {
            if (!found_dir) {
                FAIL() << "Directory should come before files";
            }
            found_file = true;
        }
    }
    EXPECT_TRUE(found_dir);
    EXPECT_TRUE(found_file);

    fs::remove_all(tmp);
}

TEST(ScannerTest, GetTotalSize) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_size";
    fs::create_directories(tmp / "sub");

    std::ofstream(tmp / "f1.txt") << "12345";       // 5 bytes
    std::ofstream(tmp / "sub" / "f2.txt") << "1234567890";  // 10 bytes

    uint64_t total = Scanner::get_total_size(tmp.string());
    EXPECT_EQ(total, 15ULL);

    fs::remove_all(tmp);
}

TEST(ScannerTest, ScanNonExistentPath) {
    Scanner scanner;
    Entry root = scanner.scan("/nonexistent/path/that/does/not/exist", 1);

    EXPECT_EQ(root.type, Entry::Type::Directory);
    EXPECT_TRUE(root.children.empty());
    EXPECT_EQ(root.total_size, 0ULL);
}

TEST(ScannerTest, CancelScan) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_cancel";
    fs::create_directories(tmp);

    // Create many files
    for (int i = 0; i < 100; ++i) {
        std::ofstream(tmp / ("file_" + std::to_string(i) + ".txt")) << "data";
    }

    Scanner scanner;
    auto root = scanner.scan(tmp.string(), 1);

    // Should have scanned all files (small enough)
    EXPECT_EQ(root.children.size(), 100u);

    fs::remove_all(tmp);
}

// ============================================================
// Integration Tests
// ============================================================

TEST(ScannerIntegration, ComplexDirectoryStructure) {
    fs::path tmp = fs::temp_directory_path() / "tachikoma_test_complex";
    fs::create_directories(tmp / "src" / "lib");
    fs::create_directories(tmp / "docs");
    fs::create_directories(tmp / "build" / "obj");

    std::ofstream(tmp / "README.md") << "# Tachikoma\nA great project\n";
    std::ofstream(tmp / "src" / "main.cpp") << "#include <iostream>\nint main() { return 0; }\n";
    std::ofstream(tmp / "src" / "lib" / "utils.cpp") << "void util() {}\n";
    std::ofstream(tmp / "docs" / "guide.md") << "# Guide\n";
    std::ofstream(tmp / "build" / "obj" / "main.o") << "binary data here!!";

    Scanner scanner;
    Entry root = scanner.scan(tmp.string(), 3);

    EXPECT_EQ(root.type, Entry::Type::Directory);
    EXPECT_GT(root.total_size, 0ULL);

    // Verify src directory has children
    bool found_src = false;
    for (const auto& child : root.children) {
        if (child.name == "src" && child.type == Entry::Type::Directory) {
            found_src = true;
            EXPECT_GT(child.total_size, 0ULL);
            EXPECT_GT(child.children.size(), 0u);
        }
    }
    EXPECT_TRUE(found_src);

    fs::remove_all(tmp);
}
