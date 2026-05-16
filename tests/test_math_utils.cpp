#include "utils/math_utils.hpp"
#include <gtest/gtest.h>

namespace to_gif {

TEST(MathUtilsTest, NormalAspectRatio) {
    EXPECT_EQ(calc_height(320, 1920, 1080), 180);
    EXPECT_EQ(calc_height(720, 1920, 1080), 406);
}

TEST(MathUtilsTest, ZeroOriginalWidthReturnsZero) {
    EXPECT_EQ(calc_height(320, 0, 1080), 0);
}

TEST(MathUtilsTest, ZeroOriginalHeightReturnsZero) {
    EXPECT_EQ(calc_height(320, 1920, 0), 0);
}

TEST(MathUtilsTest, NegativeOriginalWidthReturnsZero) {
    EXPECT_EQ(calc_height(320, -1, 1080), 0);
}

TEST(MathUtilsTest, NegativeOriginalHeightReturnsZero) {
    EXPECT_EQ(calc_height(320, 1920, -1), 0);
}

TEST(MathUtilsTest, ResultRoundedUpToEven) {
    // 321 * 1080 / 1920 = 180.5625 -> truncated to 180, already even
    EXPECT_EQ(calc_height(321, 1920, 1080), 180);
    // 322 * 1080 / 1920 = 181.125 -> truncated to 181, rounded up to 182
    EXPECT_EQ(calc_height(322, 1920, 1080), 182);
}

TEST(MathUtilsTest, SquareAspectRatio) {
    EXPECT_EQ(calc_height(100, 100, 100), 100);
    EXPECT_EQ(calc_height(101, 100, 100), 102);
}

} // namespace to_gif
