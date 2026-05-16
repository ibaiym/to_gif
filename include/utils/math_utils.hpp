#pragma once

namespace to_gif {

/**
 * 根据目标宽度、原始宽高比计算对应高度
 * 结果保证为偶数（符合视频编码常见要求）
 */
int calc_height(int width, int orig_width, int orig_height);

} // namespace to_gif
