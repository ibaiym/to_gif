#pragma once

#include "iencoder.hpp"

extern "C" {
#include <gif_lib.h>
}

namespace to_gif {

/**
 * giflib 编码器实现
 * 单一职责原则：仅负责写入 GIF 文件
 */
class GifLibEncoder : public IEncoder {
public:
    explicit GifLibEncoder(const std::string& output_path);
    ~GifLibEncoder();

    // 禁用拷贝，启用移动
    GifLibEncoder(const GifLibEncoder&) = delete;
    GifLibEncoder& operator=(const GifLibEncoder&) = delete;
    GifLibEncoder(GifLibEncoder&& other) noexcept;
    GifLibEncoder& operator=(GifLibEncoder&& other) noexcept;

    void begin(
        int width,
        int height,
        const std::vector<Color>& palette
    ) override;

    void write_frame(
        const std::vector<uint8_t>& palette_indices,
        int width,
        int height,
        int delay_cs
    ) override;

    void end() override;

private:
    void cleanup();

    std::string path_;
    GifFileType* gif_ = nullptr;
    ColorMapObject* cmap_ = nullptr;
    bool begun_ = false;
};

} // namespace to_gif
