#include "common/types.hpp"

#include <algorithm>

namespace to_gif {

void Config::validate() const {
    if (input_path.empty()) {
        throw std::invalid_argument("Input path must not be empty");
    }
    if (output_path.empty()) {
        throw std::invalid_argument("Output path must not be empty");
    }
    if (target_mb <= 0.0) {
        throw std::invalid_argument("Target size must be greater than 0");
    }
    if (quality < kMinQuality || quality > kMaxQuality) {
        throw std::invalid_argument(
            "Quality must be between " + std::to_string(kMinQuality) +
            " and " + std::to_string(kMaxQuality));
    }
    if (min_width > max_width) {
        throw std::invalid_argument(
            "min_width (" + std::to_string(min_width) +
            ") must be <= max_width (" + std::to_string(max_width) + ")");
    }
    if (min_fps > max_fps) {
        throw std::invalid_argument(
            "min_fps (" + std::to_string(min_fps) +
            ") must be <= max_fps (" + std::to_string(max_fps) + ")");
    }
    if (start_sec >= 0 && end_sec >= 0 && start_sec >= end_sec) {
        throw std::invalid_argument("start_sec must be less than end_sec");
    }

    // Validate palette color count
    bool colors_valid = std::any_of(
        std::begin(kValidColors), std::end(kValidColors),
        [&](int c) { return max_colors == c; });
    if (!colors_valid) {
        throw std::invalid_argument(
            "max_colors must be one of: 64, 128, 192, 256");
    }
}

} // namespace to_gif
