#pragma once

#include <memory>
#include "ipipeline_factory.hpp"

namespace to_gif {

/**
 * 创建默认流水线工厂
 * 使用 FFmpegDecoder + ImageQuantQuantizer + GifLibEncoder
 */
std::unique_ptr<IPipelineFactory> create_default_pipeline_factory();

} // namespace to_gif
