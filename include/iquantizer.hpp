#pragma once

#include "common/types.hpp"
#include <cstdint>
#include <vector>

namespace to_gif {

/**
 * 调色板量化器接口
 * 单一职责原则：仅负责颜色量化
 */
class IQuantizer {
public:
    virtual ~IQuantizer() = default;

    /**
     * 分析所有帧，生成全局调色板并量化
     * @param frames 输入帧（只读）
     * @param max_colors 最大颜色数
     * @param quality 量化质量
     * @return 量化后的帧数据（包含 palette_indices）
     */
    virtual std::vector<QuantizedFrame> quantize_all(
        const std::vector<Frame>& frames,
        int max_colors = 256,
        int quality = 100
    ) = 0;

    virtual const std::vector<Color>& palette() const = 0;
};

} // namespace to_gif
