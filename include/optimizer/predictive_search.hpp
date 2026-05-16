#pragma once

#include "isearch_strategy.hpp"

namespace to_gif {

/**
 * 预测搜索策略（策略模式）
 * 根据调用方提供的比例缩放当前参数：
 *   新参数 = clamp(round(当前参数 * 比例), 最小参数, 最大参数)
 */
class PredictiveSearchStrategy : public ISearchStrategy {
public:
    int predict_next(
        int current_param,
        double ratio,
        int min_param,
        int max_param
    ) const override;
};

} // namespace to_gif
