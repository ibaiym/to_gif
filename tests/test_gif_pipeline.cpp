#include "pipeline/gif_pipeline.hpp"
#include "idecoder.hpp"
#include "iquantizer.hpp"
#include "iencoder.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

namespace to_gif {

// ------------------------------------------------------------------
// Mock components
// ------------------------------------------------------------------

class MockDecoder : public IDecoder {
public:
    std::vector<Frame> extract_frames(double target_fps,
                                      int target_width,
                                      double start_sec,
                                      double end_sec) override {
        (void)target_fps;
        (void)start_sec;
        (void)end_sec;

        if (return_empty_) {
            return {};
        }

        Frame f;
        f.width = target_width;
        f.height = target_width / 2;
        f.rgba_pixels.resize(f.width * f.height * 4, 0);
        return {f, f};
    }

    double duration() const override { return 10.0; }
    int original_width() const override { return 1920; }
    int original_height() const override { return 1080; }
    double original_fps() const override { return 30.0; }

    void set_return_empty(bool v) { return_empty_ = v; }

private:
    bool return_empty_ = false;
};

class MockQuantizer : public IQuantizer {
public:
    std::vector<QuantizedFrame> quantize_all(const std::vector<Frame>& frames,
                      int max_colors,
                      int quality) override {
        (void)max_colors;
        (void)quality;

        std::vector<QuantizedFrame> quantized;
        quantized.reserve(frames.size());
        for (const auto& frame : frames) {
            QuantizedFrame qf;
            qf.width = frame.width;
            qf.height = frame.height;
            qf.palette_indices.resize(frame.width * frame.height, 0);
            quantized.push_back(std::move(qf));
        }
        return quantized;
    }

    const std::vector<Color>& palette() const override {
        static std::vector<Color> pal = {{0, 0, 0}, {255, 255, 255}};
        return pal;
    }
};

struct MockEncoderState {
    bool begin_called_ = false;
    bool end_called_ = false;
    int begin_width_ = 0;
    int begin_height_ = 0;
    size_t begin_palette_size_ = 0;
    int write_frame_count_ = 0;
    int last_width_ = 0;
    int last_height_ = 0;
    int last_delay_cs_ = 0;
};

class MockEncoder : public IEncoder {
public:
    explicit MockEncoder(MockEncoderState* state) : state_(state) {}

    void begin(int width, int height, const std::vector<Color>& palette) override {
        state_->begin_called_ = true;
        state_->begin_width_ = width;
        state_->begin_height_ = height;
        state_->begin_palette_size_ = palette.size();
    }

    void write_frame(const std::vector<uint8_t>& palette_indices,
                     int width,
                     int height,
                     int delay_cs) override {
        (void)palette_indices;
        ++state_->write_frame_count_;
        state_->last_width_ = width;
        state_->last_height_ = height;
        state_->last_delay_cs_ = delay_cs;
    }

    void end() override {
        state_->end_called_ = true;
    }

private:
    MockEncoderState* state_;
};

// ------------------------------------------------------------------
// Tests
// ------------------------------------------------------------------

TEST(GifPipelineTest, EmptyOutputPathThrows) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    EXPECT_THROW(pipeline.run(output_target, 320, 10, params), std::invalid_argument);
}

TEST(GifPipelineTest, EmptyFramesThrows) {
    MockDecoder decoder;
    decoder.set_return_empty(true);
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    EXPECT_THROW(pipeline.run(output_target, 320, 10, params), std::runtime_error);
}

TEST(GifPipelineTest, NormalFlowCallsBeginWriteEnd) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    pipeline.run(output_target, 320, 10, params);

    EXPECT_TRUE(state.begin_called_);
    EXPECT_TRUE(state.end_called_);
    EXPECT_EQ(state.write_frame_count_, 2);
    EXPECT_EQ(state.begin_width_, 320);
    EXPECT_EQ(state.begin_palette_size_, 2u);
}

TEST(GifPipelineTest, DelayIsCalculatedCorrectly) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    pipeline.run(output_target, 320, 10, params);

    EXPECT_EQ(state.last_delay_cs_, 10);
}

TEST(GifPipelineTest, DelayHasMinimumBound) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    pipeline.run(output_target, 320, 100, params);

    EXPECT_EQ(state.last_delay_cs_, 2);
}

TEST(GifPipelineTest, NonPositiveWidthThrows) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    EXPECT_THROW(pipeline.run(output_target, 0, 10, params), std::invalid_argument);
    EXPECT_THROW(pipeline.run(output_target, -1, 10, params), std::invalid_argument);
}

TEST(GifPipelineTest, NonPositiveFpsThrows) {
    MockDecoder decoder;
    MockQuantizer quantizer;
    MockEncoderState state;
    MockEncoder output_target(&state);
    GifPipeline pipeline(decoder, quantizer);
    PipelineParams params;
    params.output_path = "out.gif";
    EXPECT_THROW(pipeline.run(output_target, 320, 0, params), std::invalid_argument);
    EXPECT_THROW(pipeline.run(output_target, 320, -1, params), std::invalid_argument);
}

} // namespace to_gif
