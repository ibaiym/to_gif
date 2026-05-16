#include "utils/math_utils.hpp"
#include <cmath>

namespace to_gif {

int calc_height(int width, int orig_width, int orig_height) {
    if (orig_width <= 0 || orig_height <= 0) return 0;
    int h = static_cast<int>(
        static_cast<double>(orig_height) * width / orig_width);
    if (h % 2 != 0) ++h;
    return h;
}

} // namespace to_gif
