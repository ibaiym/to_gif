#include "pipeline/gif_pipeline.hpp"
#include "common/constants.hpp"
#include "idecoder.hpp"
#include "iquantizer.hpp"
#include "iencoder.hpp"
#include <cmath>
#include <stdexcept>

namespace to_gif {

GifPipeline::GifPipeline(IDecoder& decoder, IQuantizer& quantizer)
    : decoder_(decoder), quantizer_(quantizer) {}

void GifPipeline::run(IEncoder& output_target, int width, int fps, const PipelineParams& params) {
    if (params.output_path.empty()) {
        throw std::invalid_argument("Output path must not be empty");
    }

    if (width <= 0 || fps <= 0) {
        throw std::invalid_argument("Width and fps must be greater than 0");
    }

    auto frames = decoder_.extract_frames(
        static_cast<double>(fps), width, params.start_sec, params.end_sec);

    if (frames.empty()) {
        throw std::runtime_error("No frames extracted from video: " + params.input_path);
    }

    // Quantize frame data (returns new quantized frames, does not modify original)
    auto quantized_frames = quantizer_.quantize_all(frames, params.max_colors, params.quality);

    if (quantized_frames.empty()) {
        throw std::runtime_error("Quantization produced no frames");
    }

    output_target.begin(quantized_frames[0].width, quantized_frames[0].height, quantizer_.palette());

    int delay_cs = static_cast<int>(kCsPerSecond / static_cast<double>(fps));
    if (delay_cs < kMinDelayCs) delay_cs = kMinDelayCs;

    for (const auto& qframe : quantized_frames) {
        output_target.write_frame(qframe.palette_indices, qframe.width, qframe.height, delay_cs);
    }

    output_target.end();
}

} // namespace to_gif
