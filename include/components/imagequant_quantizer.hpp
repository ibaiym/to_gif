#pragma once

#include "iquantizer.hpp"

extern "C" {
#include <libimagequant.h>
}

#include <memory>

namespace to_gif {

// ------------------------------------------------------------------
// libimagequant C 资源的 RAII 包装器
// ------------------------------------------------------------------

struct LiqAttrDeleter {
    void operator()(liq_attr* a) const { liq_attr_destroy(a); }
};
struct LiqImageDeleter {
    void operator()(liq_image* i) const { liq_image_destroy(i); }
};
struct LiqHistogramDeleter {
    void operator()(liq_histogram* h) const { liq_histogram_destroy(h); }
};
struct LiqResultDeleter {
    void operator()(liq_result* r) const { liq_result_destroy(r); }
};

using LiqAttrPtr      = std::unique_ptr<liq_attr,      LiqAttrDeleter>;
using LiqImagePtr     = std::unique_ptr<liq_image,     LiqImageDeleter>;
using LiqHistogramPtr = std::unique_ptr<liq_histogram, LiqHistogramDeleter>;
using LiqResultPtr    = std::unique_ptr<liq_result,    LiqResultDeleter>;

/**
 * libimagequant 量化器实现
 * 单一职责原则：仅负责全局调色板生成和抖动量化
 */
class ImageQuantQuantizer : public IQuantizer {
public:
    ImageQuantQuantizer() = default;
    ~ImageQuantQuantizer() = default;

    // 禁用拷贝，启用移动
    ImageQuantQuantizer(const ImageQuantQuantizer&) = delete;
    ImageQuantQuantizer& operator=(const ImageQuantQuantizer&) = delete;
    ImageQuantQuantizer(ImageQuantQuantizer&&) = default;
    ImageQuantQuantizer& operator=(ImageQuantQuantizer&&) = default;

    std::vector<QuantizedFrame> quantize_all(
        const std::vector<Frame>& frames,
        int max_colors = 256,
        int quality = 100
    ) override;

    const std::vector<Color>& palette() const override { return palette_; }

private:
    std::vector<Color> palette_;
};

} // namespace to_gif
