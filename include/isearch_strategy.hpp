#pragma once

namespace to_gif {

/**
 * 参数搜索策略接口（策略模式）
 * 开闭原则：可通过添加新策略来扩展搜索算法
 * 依赖倒置原则：优化器依赖此接口，而非具体实现
 */
class ISearchStrategy {
public:
    virtual ~ISearchStrategy() = default;

    /**
     * 预测下一个最佳参数
     * @param current_param 当前参数值（宽度或帧率）
     * @param ratio         缩放比例（由调用方预先计算）
     * @param min_param     最小参数值
     * @param max_param     最大参数值
     * @return 预测的下一个参数值
     */
    virtual int predict_next(
        int current_param,
        double ratio,
        int min_param,
        int max_param
    ) const = 0;
};

} // namespace to_gif
