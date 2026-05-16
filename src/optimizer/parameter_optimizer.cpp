#include "optimizer/parameter_optimizer.hpp"
#include "common/constants.hpp"
#include "ipipeline_factory.hpp"
#include "pipeline/gif_pipeline.hpp"
#include "utils/logger.hpp"
#include "utils/file_utils.hpp"
#include "utils/math_utils.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>

namespace to_gif {

namespace {

/**
 * RAII temporary file guard
 * Ensures temporary file is deleted when scope ends, supports explicit release of ownership
 */
struct TempFileGuard {
private:
    std::string path_;
public:
    explicit TempFileGuard(std::string p) : path_(std::move(p)) {}
    ~TempFileGuard() {
        if (!path_.empty()) {
            std::filesystem::remove(path_);
        }
    }
    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
    TempFileGuard(TempFileGuard&&) noexcept = default;
    TempFileGuard& operator=(TempFileGuard&&) noexcept = default;

    const std::string& path() const { return path_; }

    /** Release file ownership, return path and prevent deletion on destruction */
    std::string release() {
        std::string p = std::move(path_);
        path_.clear();
        return p;
    }
};

std::string make_temp_path() {
    auto tmpdir = std::filesystem::temp_directory_path();
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::atomic<uint64_t> counter{0};
    return (tmpdir / ("to_gif_probe_" + std::to_string(now) + "_"
                      + std::to_string(++counter) + ".gif")).string();
}

double run_pipeline_and_measure(
    int width,
    int fps,
    const PipelineParams& params,
    GifPipeline& pipeline,
    IPipelineFactory& factory) {
    auto output_target = factory.create_encoder(params.output_path);
    pipeline.run(*output_target, width, fps, params);
    auto sz = file_size(params.output_path);
    if (!sz) {
        throw std::runtime_error("Failed to get output file size: " + params.output_path);
    }
    return static_cast<double>(*sz) / kBytesPerMb;
}

double probe_to_temp(
    int width,
    int fps,
    const PipelineParams& params,
    GifPipeline& pipeline,
    IPipelineFactory& factory) {
    std::string tmp = make_temp_path();
    TempFileGuard guard(tmp);
    PipelineParams probe_params = params;
    probe_params.output_path = guard.path();
    return run_pipeline_and_measure(width, fps, probe_params, pipeline, factory);
}

void print_result(
    const std::string& output_path,
    double target_mb,
    int width,
    int height,
    int fps,
    int colors,
    int quality) {
    auto sz = file_size(output_path);
    double final_size = sz ? static_cast<double>(*sz) / kBytesPerMb : 0.0;
    LOG(kInfo) << std::string(kSepLineLen, '-');
    LOG(kInfo) << "Output: " << output_path;
    LOG(kInfo) << "Size: " << final_size
               << " MB (target: " << target_mb << " MB)";
    LOG(kInfo) << "Final params: width=" << width
               << ", height=" << height
               << ", fps=" << fps
               << ", colors=" << colors
               << ", quality=" << quality;
}

/**
 * Width decay function
 * When predicted value is not less than current, force decrease using fixed decay factor kDecayFactor (default 0.85),
 * avoiding stagnation near minimum. File size is proportional to area, and area is proportional to width squared.
 */
int decay_width(int current, double ratio, int min_val, int max_val,
                ISearchStrategy& strategy) {
    int predicted = strategy.predict_next(current, ratio, min_val, max_val);
    if (predicted >= current) {
        predicted = static_cast<int>(current * kDecayFactor);
    }
    return std::max(predicted, min_val);
}

/**
 * FPS decay function
 * Similar to decay_width, but additionally ensures at least kFallbackStep (default 3fps) decrease each time,
 * and forces decrement by 1 when predicted value is not less than current, ensuring convergence toward min_val.
 */
int decay_fps(int current, double ratio, int min_val, int max_val,
              ISearchStrategy& strategy) {
    int predicted = strategy.predict_next(current, ratio, min_val, max_val);
    int next = std::max(predicted, current - kFallbackStep);
    next = std::max(next, min_val);
    if (next >= current) next = current - 1;
    return next;
}

enum class SearchPhase {
    kPrimary,
    kFallback
};

struct SearchContext {
    bool is_width;
    bool use_bisection;
    int fixed_other;
    int start_value;
    double initial_size;
    int min_val;
    int max_val;
    double target_mb;
    int max_iterations;
};

SearchContext resolve_search_context(
    bool fps_first, SearchPhase phase, const Config& cfg, double initial_size) {
    bool is_primary = (phase == SearchPhase::kPrimary);
    if (!fps_first) {
        if (is_primary) {
            return {true, true, cfg.max_fps, 0, initial_size,
                    cfg.min_width, cfg.max_width,
                    cfg.target_mb, cfg.max_resolution_iterations};
        }
        return {false, false, cfg.min_width, cfg.max_fps, initial_size,
                cfg.min_fps, cfg.max_fps,
                cfg.target_mb, cfg.max_resolution_iterations};
    }
    if (is_primary) {
        return {false, true, cfg.max_width, 0, initial_size,
                cfg.min_fps, cfg.max_fps,
                cfg.target_mb, cfg.max_resolution_iterations};
    }
    return {true, false, cfg.min_fps, cfg.max_width, initial_size,
            cfg.min_width, cfg.max_width,
            cfg.target_mb, cfg.max_resolution_iterations};
}

/**
 * Single parameter search (resolution or fps)
 *
 * Algorithm:
 * - Primary phase uses "pseudo-bisection": when probe satisfies target, probe midpoint (var + max_val) / 2
 *   toward max; when not satisfied, decay toward minimum via decay_*.
 *   Width ratio uses sqrt because file size is proportional to area, and area ∝ width².
 * - Fallback phase uses linear decay (!bisection): starting from start_value, step down,
 *   first value satisfying target is optimal, return directly.
 */
int search_param(const SearchContext& ctx,
                 const PipelineParams& params,
                 ISearchStrategy& strategy,
                 GifPipeline& pipeline,
                 IPipelineFactory& factory) {
    int min_val = ctx.min_val;
    int max_val = ctx.max_val;
    // Cannot compute ratio when initial_size <= 0, fall back to minimum directly
    if (ctx.use_bisection && ctx.initial_size <= 0.0) {
        return min_val;
    }
    // Compute target/current ratio based on parameter type: sqrt for width, linear for fps
    double ratio = ctx.is_width ? std::sqrt(ctx.target_mb / ctx.initial_size)
                                : ctx.target_mb / ctx.initial_size;
    int var = ctx.use_bisection ? strategy.predict_next(max_val, ratio, min_val, max_val)
                                : ctx.start_value;
    int iter = 0;
    int best = kNotSet;

    while (min_val <= var && var <= max_val && iter < ctx.max_iterations) {
        int w = ctx.is_width ? var : ctx.fixed_other;
        int f = ctx.is_width ? ctx.fixed_other : var;
        double sz = probe_to_temp(w, f, params, pipeline, factory);
        LOG(kDebug) << "Probe (" << w << "px, " << f << "fps): " << sz << " MB";
        if (sz <= ctx.target_mb) {
            best = var;
            if (!ctx.use_bisection) break;  // fallback phase: first satisfying value is optimal
            int next = (var + max_val) / 2; // bisection: probe toward larger value
            if (next == var) break;         // integer range exhausted
            var = next;
        } else {
            double r = ctx.is_width ? std::sqrt(ctx.target_mb / sz)
                                    : ctx.target_mb / sz;
            int next = ctx.is_width ? decay_width(var, r, min_val, max_val, strategy)
                                    : decay_fps(var, r, min_val, max_val, strategy);
            if (next == var) break;         // decay stalled, terminate search
            var = next;
        }
        ++iter;
    }
    return best;
}

void assign_search_result(
    bool is_width, int best, int fixed_other,
    int& best_w, int& best_f) {
    if (is_width) {
        best_w = best;
        best_f = fixed_other;
    } else {
        best_w = fixed_other;
        best_f = best;
    }
}

} // anonymous namespace

ParameterOptimizer::ParameterOptimizer(
    const Config& cfg,
    std::unique_ptr<ISearchStrategy> strategy,
    std::unique_ptr<IPipelineFactory> factory)
    : cfg_(cfg), strategy_(std::move(strategy)), factory_(std::move(factory)) {
    if (!strategy_) {
        throw std::invalid_argument("Search strategy cannot be null");
    }
    if (!factory_) {
        throw std::invalid_argument("Pipeline factory cannot be null");
    }
    cfg_.validate();
}

void ParameterOptimizer::log_params() const {
    LOG(kInfo) << "Target size: " << cfg_.target_mb << " MB";
    LOG(kInfo) << "Param range: width [" << cfg_.min_width << ", "
               << cfg_.max_width << "], fps [" << cfg_.min_fps << ", "
               << cfg_.max_fps << "]";
    LOG(kInfo) << "Palette: " << cfg_.max_colors
               << " colors, quality: " << cfg_.quality;
    LOG(kInfo) << std::string(kTitleLineLen, '=');
}

PipelineParams ParameterOptimizer::make_pipeline_params() const {
    return {cfg_.input_path, cfg_.output_path,
            cfg_.start_sec, cfg_.end_sec,
            cfg_.max_colors, cfg_.quality};
}

double ParameterOptimizer::try_max_params(
    int& out_w, int& out_f, int orig_width, int orig_height,
    GifPipeline& pipeline) {
    PipelineParams params = make_pipeline_params();
    std::string tmp = make_temp_path();
    TempFileGuard guard(tmp);
    PipelineParams probe_params = params;
    probe_params.output_path = guard.path();
    double size = run_pipeline_and_measure(cfg_.max_width, cfg_.max_fps, probe_params,
                                           pipeline, *factory_);
    LOG(kInfo) << "Probe (max: " << cfg_.max_width << "px, "
               << cfg_.max_fps << "fps): " << size << " MB";

    if (size <= cfg_.target_mb) {
        try {
            std::filesystem::rename(guard.path(), params.output_path);
        } catch (const std::filesystem::filesystem_error&) {
            std::filesystem::copy_file(guard.path(), params.output_path,
                                       std::filesystem::copy_options::overwrite_existing);
        }
        guard.release();
        print_result(params.output_path, cfg_.target_mb, cfg_.max_width,
                     calc_height(cfg_.max_width, orig_width, orig_height),
                     cfg_.max_fps, cfg_.max_colors, cfg_.quality);
        LOG(kInfo) << "Max parameters satisfy the limit, "
                   << "outputting best quality directly!";
        out_w = cfg_.max_width;
        out_f = cfg_.max_fps;
    }
    return size;
}

bool ParameterOptimizer::search_primary_params(
    int& best_w, int& best_f, double initial_size,
    GifPipeline& pipeline) {
    bool fps_first = (cfg_.search_priority ==
                      SearchPriority::kFpsFirst);
    auto ctx = resolve_search_context(fps_first, SearchPhase::kPrimary, cfg_, initial_size);
    PipelineParams params = make_pipeline_params();

    int best = search_param(ctx, params,
                            *strategy_, pipeline, *factory_);
    if (best == kNotSet) {
        return false;
    }

    assign_search_result(ctx.is_width, best, ctx.fixed_other,
                         best_w, best_f);
    LOG(kInfo) << "Locked best "
               << (fps_first ? "fps" : "resolution")
               << ": " << best_w << "px @ " << best_f
               << "fps";
    return true;
}

bool ParameterOptimizer::search_fallback_params(
    int& best_w, int& best_f,
    GifPipeline& pipeline) {
    bool fps_first = (cfg_.search_priority ==
                      SearchPriority::kFpsFirst);

    LOG(kInfo) << (fps_first
        ? "FPS already reduced to minimum (" +
          std::to_string(cfg_.min_fps) +
          "fps) still exceeds limit, reducing width..."
        : "Resolution already reduced to minimum (" +
          std::to_string(cfg_.min_width) +
          "px) still exceeds limit, reducing fps...");

    auto ctx = resolve_search_context(fps_first, SearchPhase::kFallback, cfg_, 0.0);
    PipelineParams params = make_pipeline_params();

    int best = search_param(ctx, params,
                            *strategy_, pipeline, *factory_);
    if (best == kNotSet) {
        return false;
    }

    assign_search_result(ctx.is_width, best, ctx.fixed_other,
                         best_w, best_f);
    return true;
}

void ParameterOptimizer::finalize(
    int width, int fps, int orig_width, int orig_height,
    GifPipeline& pipeline) const {
    PipelineParams params = make_pipeline_params();
    run_pipeline_and_measure(width, fps, params, pipeline, *factory_);
    print_result(cfg_.output_path, cfg_.target_mb, width,
                 calc_height(width, orig_width, orig_height),
                 fps, cfg_.max_colors, cfg_.quality);
}

std::pair<int, int> ParameterOptimizer::optimize() {
    log_params();

    auto decoder = factory_->create_decoder(cfg_.input_path);
    auto quantizer = factory_->create_quantizer();
    GifPipeline pipeline(*decoder, *quantizer);
    int orig_width = decoder->original_width();
    int orig_height = decoder->original_height();

    int best_w = kNotSet;
    int best_f = kNotSet;

    double size = try_max_params(best_w, best_f, orig_width, orig_height, pipeline);
    if (best_w != kNotSet) {
        return {best_w, best_f};
    }

    // Phase 1: search primary parameter
    if (search_primary_params(best_w, best_f, size, pipeline)) {
        finalize(best_w, best_f, orig_width, orig_height, pipeline);
        return {best_w, best_f};
    }

    // Phase 2: fallback search for secondary parameter
    if (search_fallback_params(best_w, best_f, pipeline)) {
        finalize(best_w, best_f, orig_width, orig_height, pipeline);
        return {best_w, best_f};
    }

    throw std::runtime_error(
        std::string("Even minimum parameters (") + std::to_string(cfg_.min_width) +
        "px, " + std::to_string(cfg_.min_fps) +
        "fps) cannot compress below " + std::to_string(cfg_.target_mb) +
        "MB.\nSuggestion: trim video duration or relax the limit.");
}

} // namespace to_gif
