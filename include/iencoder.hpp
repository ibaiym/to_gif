#pragma once

#include "common/types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace to_gif {

/**
 * GIF 输出目标接口
 *
 * 设计说明：
 * 本接口代表"一次 GIF 写入操作的目标"。
 * 实现类应在 begin() 时绑定具体输出路径，在 end() 时完成写入。
 * 由于一次写入对应一个输出文件，本接口实例通常不作为长期设施复用。
 */
class IEncoder {
public:
    virtual ~IEncoder() = default;

    /**
     * 开始写入 GIF
     */
    virtual void begin(
        int width,
        int height,
        const std::vector<Color>& palette
    ) = 0;

    /**
     * 写入一帧
     * @param palette_indices 调色板索引数据
     * @param delay_cs 帧延迟（百分之一秒）
     */
    virtual void write_frame(
        const std::vector<uint8_t>& palette_indices,
        int width,
        int height,
        int delay_cs
    ) = 0;

    /**
     * 结束写入
     */
    virtual void end() = 0;
};

} // namespace to_gif
