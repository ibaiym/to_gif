#include "optimizer/predictive_search.hpp"
#include "common/constants.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace to_gif {

int PredictiveSearchStrategy::predict_next(
    int current_param,
    double ratio,
    int min_param,
    int max_param) const {

    // Validate parameter validity
    if (min_param > max_param) {
        throw std::invalid_argument(
            "min_param (" + std::to_string(min_param) + 
            ") must be <= max_param (" + std::to_string(max_param) + ")");
    }

    // When ratio is invalid, use conservative strategy: halve current parameter
    if (ratio <= 0.0 || std::isnan(ratio) || std::isinf(ratio)) {
        int reduced = std::max(current_param / 2, min_param);
        return std::clamp(reduced, min_param, max_param);
    }

    // Normal prediction: adjust parameter based on ratio
    int predicted = static_cast<int>(std::lround(current_param * ratio));
    return std::clamp(predicted, min_param, max_param);
}

} // namespace to_gif
