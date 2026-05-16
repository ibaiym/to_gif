#include "common/types.hpp"
#include <gtest/gtest.h>

namespace to_gif {

TEST(ConfigValidateTest, EmptyInputPathThrows) {
    Config cfg;
    cfg.input_path = "";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, EmptyOutputPathThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "";
    cfg.target_mb = 5.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, NonPositiveTargetMbThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 0.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
    cfg.target_mb = -1.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, QualityOutOfRangeThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.quality = 0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
    cfg.quality = 101;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, MinWidthGreaterThanMaxThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.min_width = 640;
    cfg.max_width = 320;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, MinFpsGreaterThanMaxThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.min_fps = 30;
    cfg.max_fps = 20;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, InvalidStartEndTimeThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.start_sec = 5.0;
    cfg.end_sec = 3.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
    cfg.start_sec = 5.0;
    cfg.end_sec = 5.0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, InvalidColorsThrows) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.max_colors = 100;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
    cfg.max_colors = 0;
    EXPECT_THROW(cfg.validate(), std::invalid_argument);
}

TEST(ConfigValidateTest, ValidConfigPasses) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    cfg.quality = 80;
    cfg.min_width = 320;
    cfg.max_width = 720;
    cfg.min_fps = 5;
    cfg.max_fps = 20;
    cfg.start_sec = 2.0;
    cfg.end_sec = 8.0;
    cfg.max_colors = 128;
    EXPECT_NO_THROW(cfg.validate());
}

TEST(ConfigValidateTest, DefaultConfigValid) {
    Config cfg;
    cfg.input_path = "in.mp4";
    cfg.output_path = "out.gif";
    cfg.target_mb = 5.0;
    EXPECT_NO_THROW(cfg.validate());
}

} // namespace to_gif
