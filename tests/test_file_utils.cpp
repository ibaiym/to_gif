#include "utils/file_utils.hpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace to_gif {

TEST(FileUtilsTest, ExistingFileReturnsCorrectSize) {
    const char* path = "/tmp/to_gif_test_file.txt";
    const char* content = "hello_world";

    std::ofstream ofs(path, std::ios::binary);
    ofs << content;
    ofs.close();

    auto sz = file_size(path);
    ASSERT_TRUE(sz.has_value());
    EXPECT_EQ(*sz, std::strlen(content));

    std::remove(path);
}

TEST(FileUtilsTest, NonExistentFileReturnsNullopt) {
    auto sz = file_size("/tmp/to_gif_nonexistent_file_xyz");
    EXPECT_FALSE(sz.has_value());
}

TEST(FileUtilsTest, EmptyFileReturnsZero) {
    const char* path = "/tmp/to_gif_empty_file.txt";
    std::ofstream ofs(path, std::ios::binary);
    ofs.close();

    auto sz = file_size(path);
    ASSERT_TRUE(sz.has_value());
    EXPECT_EQ(*sz, 0u);

    std::remove(path);
}

} // namespace to_gif
