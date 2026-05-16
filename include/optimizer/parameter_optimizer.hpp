#pragma once

#include "common/types.hpp"
#include "ipipeline_factory.hpp"
#include "isearch_strategy.hpp"
#include <memory>
#include <utility>

namespace to_gif {

class GifPipeline;

/**
 * 参数优化器
 * 单一职责原则：在目标大小限制下搜索最优参数
 */
class ParameterOptimizer {
public:
    explicit ParameterOptimizer(
        const Config& cfg,
        std::unique_ptr<ISearchStrategy> strategy,
        std::unique_ptr<IPipelineFactory> factory
    );

    /**
     * 执行参数优化
     * 输入/输出路径从 cfg_.input_path / cfg_.output_path 读取
     */
    std::pair<int, int> optimize();

private:
    void log_params() const;
    PipelineParams make_pipeline_params() const;
    double try_max_params(int& out_w, int& out_f, int orig_width, int orig_height,
                          GifPipeline& pipeline);
    bool search_primary_params(int& best_w, int& best_f, double initial_size,
                               GifPipeline& pipeline);
    bool search_fallback_params(int& best_w, int& best_f,
                                GifPipeline& pipeline);
    void finalize(int width, int fps, int orig_width, int orig_height,
                  GifPipeline& pipeline) const;

    Config cfg_;
    std::unique_ptr<ISearchStrategy> strategy_;
    std::unique_ptr<IPipelineFactory> factory_;
};

} // namespace to_gif
