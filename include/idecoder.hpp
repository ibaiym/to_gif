#pragma once

#include "common/types.hpp"
#include <string>
#include <vector>

namespace to_gif {

/**
 * 视频解码器接口（I - 接口隔离）
 * 单一职责原则：仅负责从视频文件中提取帧
 */
class IDecoder {
public:
    virtual ~IDecoder() = default;

    /**
     * 提取视频帧
     */
    virtual std::vector<Frame> extract_frames(
        double target_fps,
        int target_width,
        double start_sec = -1.0,
        double end_sec = -1.0
    ) = 0;

    // 元数据
    virtual double duration() const = 0;
    virtual int original_width() const = 0;
    virtual int original_height() const = 0;
    virtual double original_fps() const = 0;
};

} // namespace to_gif
