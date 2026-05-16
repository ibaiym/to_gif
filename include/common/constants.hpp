#pragma once

#include <cstddef>
#include <cstdint>

namespace to_gif {

// ------------------------------------------------------------------
// 全局常量定义
// ------------------------------------------------------------------

// 文件大小转换
inline constexpr double kBytesPerMb = 1024.0 * 1024.0;
inline constexpr double kTargetMbUnset = -1.0;

// 参数默认值和范围
inline constexpr int kNotSet = -1;
inline constexpr int kMinQuality = 1;
inline constexpr int kMaxQuality = 100;
inline constexpr int kDefaultMaxWidth = 720;
inline constexpr int kDefaultMaxFps = 20;
inline constexpr int kDefaultMinWidth = 320;
inline constexpr int kDefaultMinFps = 5;
inline constexpr int kDefaultMaxColors = 256;
inline constexpr int kDefaultQuality = 100;
inline constexpr int kDefaultMaxResolutionIterations = 6;

// 有效的调色板颜色数
inline constexpr int kValidColors[] = {64, 128, 192, 256};
inline constexpr size_t kValidColorsCount = 4;

// GIF 编码相关常量
inline constexpr int kMinDelayCs = 2;
inline constexpr double kCsPerSecond = 100.0;

// 帧处理相关常量
inline constexpr int kBytesPerPixel = 4;

// 搜索策略相关常量
inline constexpr double kDecayFactor = 0.85;
inline constexpr int kFallbackStep = 3;

// 日志格式相关常量
inline constexpr int kTitleLineLen = 60;
inline constexpr int kSepLineLen = 60;

} // namespace to_gif
