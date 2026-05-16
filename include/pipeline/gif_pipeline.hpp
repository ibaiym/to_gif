#pragma once

#include "common/types.hpp"
#include "idecoder.hpp"
#include "iquantizer.hpp"
#include "iencoder.hpp"

namespace to_gif {

/**
 * GIF 转换流水线
 * 单一职责原则：接收组件引用和配置，自主完成完整转换
 */
class GifPipeline {
public:
    GifPipeline(IDecoder& decoder, IQuantizer& quantizer);

    // 禁用拷贝，允许移动
    GifPipeline(const GifPipeline&) = delete;
    GifPipeline& operator=(const GifPipeline&) = delete;
    GifPipeline(GifPipeline&&) = default;
    GifPipeline& operator=(GifPipeline&&) = default;

    /**
     * 执行完整转换
     *
     * 生命周期模型：
     * - decoder_/quantizer_ 为长期持有的输入处理设施（构造注入）
     * - output_target 为本次操作绑定的输出目标（运行时传入）
     */
    void run(IEncoder& output_target, int width, int fps, const PipelineParams& params);

private:
    IDecoder& decoder_;
    IQuantizer& quantizer_;
};

} // namespace to_gif
