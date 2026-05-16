#include "factories/default_pipeline_factory.hpp"
#include "components/ffmpeg_decoder.hpp"
#include "components/imagequant_quantizer.hpp"
#include "components/giflib_encoder.hpp"

namespace to_gif {

namespace {

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

} // anonymous namespace

std::unique_ptr<IPipelineFactory> create_default_pipeline_factory() {
    return std::make_unique<DefaultPipelineFactory>();
}

} // namespace to_gif
