#pragma once

#include "idecoder.hpp"
#include <memory>
#include <optional>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace to_gif {

// FFmpeg 资源的 RAII 包装器
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) avformat_close_input(&ctx);
    }
};
struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) avcodec_free_context(&ctx);
    }
};
struct SwsContextDeleter {
    void operator()(SwsContext* ctx) const {
        if (ctx) sws_freeContext(ctx);
    }
};
struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) av_frame_free(&frame);
    }
};
struct AVBufferDeleter {
    void operator()(uint8_t* buffer) const {
        if (buffer) av_free(buffer);
    }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVBufferPtr = std::unique_ptr<uint8_t[], AVBufferDeleter>;

/**
 * FFmpeg 解码器实现（里氏替换原则：实现 IDecoder 接口）
 * 单一职责原则：仅负责视频解码和帧提取
 */
class FFmpegDecoder : public IDecoder {
public:
    explicit FFmpegDecoder(const std::string& path);
    ~FFmpegDecoder();

    // 禁用拷贝，启用移动
    FFmpegDecoder(const FFmpegDecoder&) = delete;
    FFmpegDecoder& operator=(const FFmpegDecoder&) = delete;
    FFmpegDecoder(FFmpegDecoder&& other) noexcept;
    FFmpegDecoder& operator=(FFmpegDecoder&& other) noexcept;

    std::vector<Frame> extract_frames(
        double target_fps,
        int target_width,
        double start_sec = -1.0,
        double end_sec = -1.0
    ) override;

    double duration() const override;
    int original_width() const override;
    int original_height() const override;
    double original_fps() const override;

private:
    void open_stream();
    void setup_scaler(int src_w, int src_h, int dst_w, int dst_h);
    void setup_rgb_buffer(int width, int height);
    std::optional<Frame> decode_next_frame();
    Frame convert_frame();
    void reset() noexcept;

    std::string path_;
    AVFormatContextPtr fmt_ctx_;
    AVCodecContextPtr codec_ctx_;
    SwsContextPtr sws_ctx_;
    AVFramePtr frame_;
    AVFramePtr rgb_frame_;
    AVBufferPtr rgb_buffer_;
    int video_stream_idx_ = -1;
    double duration_ = 0.0;
    double fps_ = 0.0;
    int orig_width_ = 0;
    int orig_height_ = 0;
    int scaler_dst_width_ = 0;
    int scaler_dst_height_ = 0;
};

} // namespace to_gif
