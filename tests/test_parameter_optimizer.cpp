#include "optimizer/parameter_optimizer.hpp"
#include "optimizer/predictive_search.hpp"
#include "ipipeline_factory.hpp"
#include "components/ffmpeg_decoder.hpp"
#include "components/imagequant_quantizer.hpp"
#include "components/giflib_encoder.hpp"
#include <gtest/gtest.h>

namespace to_gif {

class DefaultPipelineFactory : public IPipelineFactory {
public:
    std::unique_ptr<IDecoder> create_decoder(
        const std::string& path) override {
        return std::make_unique<FFmpegDecoder>(path);
    }
    std::unique_ptr<IQuantizer> create_quantizer() override {
        return std::make_unique<ImageQuantQuantizer>();
    }
    std::unique_ptr<IEncoder> create_encoder(
        const std::string& path) override {
        return std::make_unique<GifLibEncoder>(path);
    }
};

TEST(ParameterOptimizerTest, MaxParamsSatisfyLimit) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out.gif";
    cfg.target_mb = 100.0;
    cfg.min_width = 320;
    cfg.max_width = 720;
    cfg.min_fps = 5;
    cfg.max_fps = 20;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    auto factory = std::make_unique<DefaultPipelineFactory>();
    ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));

    auto result = optimizer.optimize();
    EXPECT_EQ(result.first, cfg.max_width);
    EXPECT_EQ(result.second, cfg.max_fps);
}

TEST(ParameterOptimizerTest, MinParamsExceedsLimitThrows) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out2.gif";
    cfg.target_mb = 0.001;  // 1KB, impossible to achieve
    cfg.min_width = 320;
    cfg.max_width = 320;
    cfg.min_fps = 5;
    cfg.max_fps = 5;
    cfg.max_colors = 64;
    cfg.quality = 1;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    auto factory = std::make_unique<DefaultPipelineFactory>();
    ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));

    EXPECT_THROW(optimizer.optimize(), std::runtime_error);
}

TEST(ParameterOptimizerTest, PrimarySearchReducesResolution) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out3.gif";
    cfg.target_mb = 5.0;  // Below max parameter output (~9.15MB), triggers search
    cfg.min_width = 320;
    cfg.max_width = 720;
    cfg.min_fps = 5;
    cfg.max_fps = 20;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    auto factory = std::make_unique<DefaultPipelineFactory>();
    ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));

    auto result = optimizer.optimize();
    // Max parameters (720px/20fps) exceed 5MB, search should reduce resolution or fps
    EXPECT_LT(result.first, cfg.max_width);
}

TEST(ParameterOptimizerTest, FpsFirstSearchReducesFps) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out4.gif";
    cfg.target_mb = 5.0;
    cfg.min_width = 320;
    cfg.max_width = 720;
    cfg.min_fps = 5;
    cfg.max_fps = 20;
    cfg.search_priority = SearchPriority::kFpsFirst;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    auto factory = std::make_unique<DefaultPipelineFactory>();
    ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));

    auto result = optimizer.optimize();
    // kFpsFirst should prioritize reducing fps
    EXPECT_LT(result.second, cfg.max_fps);
}

TEST(ParameterOptimizerTest, FallbackSearchFixedResolution) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out5.gif";
    cfg.target_mb = 5.0;
    // Fix resolution, force entry into fallback phase to search fps
    cfg.min_width = 720;
    cfg.max_width = 720;
    cfg.min_fps = 5;
    cfg.max_fps = 20;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    auto factory = std::make_unique<DefaultPipelineFactory>();
    ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));

    auto result = optimizer.optimize();
    EXPECT_EQ(result.first, 720);
    // Fallback phase should reduce fps to meet target size
    EXPECT_LT(result.second, cfg.max_fps);
}

TEST(ParameterOptimizerTest, NullStrategyThrows) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out6.gif";
    cfg.target_mb = 5.0;
    cfg.validate();

    auto factory = std::make_unique<DefaultPipelineFactory>();
    EXPECT_THROW(
        ParameterOptimizer(cfg, nullptr, std::move(factory)),
        std::invalid_argument);
}

TEST(ParameterOptimizerTest, NullFactoryThrows) {
    Config cfg;
    cfg.input_path = "../data/oceans_22_to_26.mp4";
    cfg.output_path = "to_gif_test_opt_out7.gif";
    cfg.target_mb = 5.0;
    cfg.validate();

    auto strategy = std::make_unique<PredictiveSearchStrategy>();
    EXPECT_THROW(
        ParameterOptimizer(cfg, std::move(strategy), nullptr),
        std::invalid_argument);
}

} // namespace to_gif
