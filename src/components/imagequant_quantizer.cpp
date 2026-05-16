#include "components/imagequant_quantizer.hpp"
#include "utils/logger.hpp"
#include <stdexcept>

namespace to_gif {

std::vector<QuantizedFrame> ImageQuantQuantizer::quantize_all(
    const std::vector<Frame>& frames,
    int max_colors,
    int quality) {

    if (frames.empty()) {
        throw std::invalid_argument("No frames to quantize");
    }

    LiqAttrPtr local_attr(liq_attr_create());
    if (!local_attr) {
        throw std::runtime_error("Failed to create libimagequant attributes");
    }
    liq_attr* attr_raw = local_attr.get();
    liq_set_max_colors(attr_raw, max_colors);
    liq_set_quality(attr_raw, kMinQuality, quality);

    const int width = frames[0].width;
    const int height = frames[0].height;

    // Collect color data from all frames into histogram
    LiqHistogramPtr hist(liq_histogram_create(attr_raw));
    if (!hist) {
        throw std::runtime_error("Failed to create histogram for global palette analysis");
    }

    for (size_t i = 0; i < frames.size(); ++i) {
        LiqImagePtr img(liq_image_create_rgba(
            attr_raw, frames[i].rgba_pixels.data(), width, height, 0.0));
        if (!img) {
            throw std::runtime_error(
                "Failed to create libimagequant image for frame " + std::to_string(i) +
                ", dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        }
        liq_histogram_add_image(hist.get(), attr_raw, img.get());
    }

    // Generate global palette from histogram
    liq_result* result_raw = nullptr;
    if (liq_histogram_quantize(hist.get(), attr_raw, &result_raw) != LIQ_OK) {
        result_raw = nullptr;
    }
    LiqResultPtr result(result_raw);

    if (!result) {
        // Fallback: quantize using first frame directly
        LOG(kWarn) << "Global palette generation failed, falling back to single-frame quantization";
        LiqImagePtr first_img(liq_image_create_rgba(
            attr_raw, frames[0].rgba_pixels.data(), width, height, 0.0));
        if (first_img) {
            result.reset(liq_quantize_image(attr_raw, first_img.get()));
        }
    }

    if (!result) {
        throw std::runtime_error(
            "Quantization failed: unable to generate palette from " + 
            std::to_string(frames.size()) + " frames. " +
            "Try reducing max_colors or checking input video format.");
    }

    // Extract palette
    const liq_palette* pal = liq_get_palette(result.get());
    palette_.resize(pal->count);
    for (unsigned int i = 0; i < pal->count; ++i) {
        const liq_color& c = pal->entries[i];
        palette_[i] = Color{c.r, c.g, c.b, c.a};
    }

    // Map each frame using global palette, create new quantized frames
    std::vector<QuantizedFrame> quantized_frames;
    quantized_frames.reserve(frames.size());

    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        LiqImagePtr img(liq_image_create_rgba(
            attr_raw, frame.rgba_pixels.data(), frame.width, frame.height, 0.0));
        if (!img) {
            throw std::runtime_error(
                "Failed to create image for remapping frame " + std::to_string(i));
        }

        // Create new quantized frame
        QuantizedFrame quantized_frame;
        quantized_frame.width = frame.width;
        quantized_frame.height = frame.height;

        // Ensure index output buffer size is correct
        const size_t expected_size = static_cast<size_t>(frame.width) * frame.height;
        quantized_frame.palette_indices.resize(expected_size);

        liq_error remap_err = liq_write_remapped_image(
            result.get(), img.get(),
            quantized_frame.palette_indices.data(),
            quantized_frame.palette_indices.size());
        if (remap_err != LIQ_OK) {
            throw std::runtime_error(
                "Global palette frame remapping failed for frame " + std::to_string(i) +
                ", error code: " + std::to_string(remap_err));
        }

        quantized_frames.push_back(std::move(quantized_frame));
    }

    return quantized_frames;
}

} // namespace to_gif
