#pragma once

#include "common/constants.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

namespace to_gif {

/**
 * 单帧 RGBA 数据
 */
struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

struct FrameBase {
    int width = 0;
    int height = 0;
};

struct Frame : FrameBase {
    std::vector<uint8_t> rgba_pixels;      // 原始 RGBA 像素数据，宽度 × 高度 × 4 字节
};

struct QuantizedFrame : FrameBase {
    std::vector<uint8_t> palette_indices;  // 量化后的调色板索引数据，宽度 × 高度 × 1 字节
};

/**
 * 转换配置
 */
enum class SearchPriority {
    kResolutionFirst,  // 先搜索分辨率，再降低帧率
    kFpsFirst          // 先搜索帧率，再降低分辨率
};

struct Config {
    double target_mb = kTargetMbUnset;   // 目标大小（MB）
    int max_width = kDefaultMaxWidth;    // 最大输出宽度
    int max_fps = kDefaultMaxFps;        // 最大输出帧率
    int min_width = kDefaultMinWidth;    // 最小输出宽度
    int min_fps = kDefaultMinFps;        // 最小输出帧率
    int max_colors = kDefaultMaxColors;  // 调色板颜色数量
    int quality = kDefaultQuality;       // libimagequant 质量
    double start_sec = -1.0;             // 裁剪起始时间
    double end_sec = -1.0;               // 裁剪结束时间
    int max_resolution_iterations = kDefaultMaxResolutionIterations; // 首要参数搜索的最大迭代次数
    SearchPriority search_priority = SearchPriority::kResolutionFirst; // 搜索策略优先级
    std::string input_path;              // 输入视频路径
    std::string output_path;             // 输出 GIF 路径

    /**
     * 验证配置的合法性
     * @throws std::invalid_argument 如果配置无效
     */
    void validate() const;
};

/**
 * GIF 流水线运行参数
 * 从 Config 中提取的、Pipeline 实际需要的子集
 */
struct PipelineParams {
    std::string input_path;
    std::string output_path;
    double start_sec = -1.0;
    double end_sec = -1.0;
    int max_colors = kDefaultMaxColors;
    int quality = kDefaultQuality;
};

} // namespace to_gif
