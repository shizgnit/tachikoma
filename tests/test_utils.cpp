#include <gtest/gtest.h>
#include "filesystem/entry.hpp"

using namespace tachikoma::filesystem;

// ============================================================
// Size Formatting Edge Cases
// ============================================================

TEST(UtilsTest, FormatSizeZero) {
    EXPECT_EQ(Entry::format_size(0), "0.00 B");
}

TEST(UtilsTest, FormatSizeOneByte) {
    EXPECT_EQ(Entry::format_size(1), "1.00 B");
}

TEST(UtilsTest, FormatSizeJustBelowKB) {
    EXPECT_EQ(Entry::format_size(1023), "1023.00 B");
}

TEST(UtilsTest, FormatSizeExactKB) {
    EXPECT_EQ(Entry::format_size(1024), "1.00 KB");
}

TEST(UtilsTest, FormatSizeFractionalKB) {
    auto result = Entry::format_size(1536);
    EXPECT_EQ(result, "1.50 KB");
}

TEST(UtilsTest, FormatSizeExactMB) {
    EXPECT_EQ(Entry::format_size(1048576), "1.00 MB");
}

TEST(UtilsTest, FormatSizeExactGB) {
    EXPECT_EQ(Entry::format_size(1073741824ULL), "1.00 GB");
}

TEST(UtilsTest, FormatSizeExactTB) {
    EXPECT_EQ(Entry::format_size(1099511627776ULL), "1.00 TB");
}

TEST(UtilsTest, FormatSizePetabytes) {
    auto result = Entry::format_size(1125899906842624ULL);
    EXPECT_EQ(result, "1.00 PB");
}

TEST(UtilsTest, FormatSizeLargeValue) {
    // Very large value should cap at PB
    auto result = Entry::format_size(UINT64_MAX);
    EXPECT_NE(result, "");
    EXPECT_TRUE(result.find("PB") != std::string::npos ||
                result.find("EB") != std::string::npos);
}

// ============================================================
// Entry Display Name
// ============================================================

TEST(UtilsTest, DisplayNameEmptyDir) {
    Entry e;
    e.name = "";
    e.type = Entry::Type::Directory;
    EXPECT_EQ(e.display_name(), "[  ]");
}

TEST(UtilsTest, DisplayNameLongName) {
    Entry e;
    e.name = std::string(100, 'a');
    e.type = Entry::Type::File;
    EXPECT_EQ(e.display_name(), std::string(100, 'a'));
}

TEST(UtilsTest, DisplayNameSpecialChars) {
    Entry e;
    e.name = "file-with_special.chars.txt";
    e.type = Entry::Type::File;
    EXPECT_EQ(e.display_name(), "file-with_special.chars.txt");
}

TEST(UtilsTest, DisplayNameDirectoryWithSpaces) {
    Entry e;
    e.name = "My Documents";
    e.type = Entry::Type::Directory;
    EXPECT_EQ(e.display_name(), "[ My Documents ]");
}

// ============================================================
// Entry Type Handling
// ============================================================

TEST(UtilsTest, FileType) {
    Entry e;
    e.type = Entry::Type::File;
    e.size = 1024;
    EXPECT_EQ(e.type, Entry::Type::File);
    EXPECT_EQ(e.size, 1024ULL);
}

TEST(UtilsTest, DirectoryType) {
    Entry e;
    e.type = Entry::Type::Directory;
    e.total_size = 2048;
    EXPECT_EQ(e.type, Entry::Type::Directory);
    EXPECT_EQ(e.total_size, 2048ULL);
}

TEST(UtilsTest, SymlinkType) {
    Entry e;
    e.type = Entry::Type::Symlink;
    e.size = 0;
    EXPECT_EQ(e.type, Entry::Type::Symlink);
}

TEST(UtilsTest, UnknownType) {
    Entry e;
    EXPECT_EQ(e.type, Entry::Type::Unknown);
}

// ============================================================
// Entry State
// ============================================================

TEST(UtilsTest, ExpandedState) {
    Entry e;
    EXPECT_FALSE(e.expanded);
    e.expanded = true;
    EXPECT_TRUE(e.expanded);
}

TEST(UtilsTest, ChildrenVector) {
    Entry parent;
    parent.type = Entry::Type::Directory;

    Entry child1;
    child1.name = "file1.txt";
    child1.type = Entry::Type::File;
    child1.size = 100;

    Entry child2;
    child2.name = "file2.txt";
    child2.type = Entry::Type::File;
    child2.size = 200;

    parent.children.push_back(std::move(child1));
    parent.children.push_back(std::move(child2));

    EXPECT_EQ(parent.children.size(), 2u);
    EXPECT_EQ(parent.children[0].name, "file1.txt");
    EXPECT_EQ(parent.children[1].name, "file2.txt");
}
