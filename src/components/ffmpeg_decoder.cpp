#include "components/ffmpeg_decoder.hpp"
#include "utils/math_utils.hpp"
#include <cstring>
#include <stdexcept>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace {

// RAII wrapper for AVPacket
struct AVPacketGuard {
    AVPacket* packet = nullptr;
    explicit AVPacketGuard(AVPacket* p) : packet(p) {}
    ~AVPacketGuard() {
        if (packet) {
            av_packet_free(&packet);
        }
    }
    AVPacketGuard(const AVPacketGuard&) = delete;
    AVPacketGuard& operator=(const AVPacketGuard&) = delete;
};

} // anonymous namespace

namespace to_gif {

FFmpegDecoder::FFmpegDecoder(const std::string& path) : path_(path) {
    open_stream();
}

FFmpegDecoder::~FFmpegDecoder() {
    // Smart pointers handle cleanup automatically
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoder&& other) noexcept
    : path_(std::move(other.path_)),
      fmt_ctx_(std::move(other.fmt_ctx_)),
      codec_ctx_(std::move(other.codec_ctx_)),
      sws_ctx_(std::move(other.sws_ctx_)),
      frame_(std::move(other.frame_)),
      rgb_frame_(std::move(other.rgb_frame_)),
      rgb_buffer_(std::move(other.rgb_buffer_)),
      video_stream_idx_(other.video_stream_idx_),
      duration_(other.duration_),
      fps_(other.fps_),
      orig_width_(other.orig_width_),
      orig_height_(other.orig_height_),
      scaler_dst_width_(other.scaler_dst_width_),
      scaler_dst_height_(other.scaler_dst_height_) {
    other.reset();
}

FFmpegDecoder& FFmpegDecoder::operator=(FFmpegDecoder&& other) noexcept {
    if (this != &other) {
        path_ = std::move(other.path_);
        fmt_ctx_ = std::move(other.fmt_ctx_);
        codec_ctx_ = std::move(other.codec_ctx_);
        sws_ctx_ = std::move(other.sws_ctx_);
        frame_ = std::move(other.frame_);
        rgb_frame_ = std::move(other.rgb_frame_);
        rgb_buffer_ = std::move(other.rgb_buffer_);
        video_stream_idx_ = other.video_stream_idx_;
        duration_ = other.duration_;
        fps_ = other.fps_;
        orig_width_ = other.orig_width_;
        orig_height_ = other.orig_height_;
        scaler_dst_width_ = other.scaler_dst_width_;
        scaler_dst_height_ = other.scaler_dst_height_;
        other.reset();
    }
    return *this;
}

void FFmpegDecoder::reset() noexcept {
    video_stream_idx_ = -1;
    duration_ = 0.0;
    fps_ = 0.0;
    orig_width_ = 0;
    orig_height_ = 0;
    scaler_dst_width_ = 0;
    scaler_dst_height_ = 0;
}

void FFmpegDecoder::open_stream() {
    // Use RAII to ensure exception safety
    AVFormatContext* raw_fmt_ctx = nullptr;
    int ret = avformat_open_input(&raw_fmt_ctx, path_.c_str(), nullptr, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(
            "Failed to open video file: " + path_ + " (error: " + errbuf + ")");
    }
    fmt_ctx_.reset(raw_fmt_ctx);

    ret = avformat_find_stream_info(fmt_ctx_.get(), nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(
            "Failed to get video stream info: " + path_ + " (error: " + errbuf + ")");
    }

    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; ++i) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx_ = static_cast<int>(i);
            break;
        }
    }
    if (video_stream_idx_ == -1) {
        throw std::runtime_error("No video stream found in: " + path_);
    }

    AVStream* stream = fmt_ctx_->streams[video_stream_idx_];
    AVCodecParameters* codecpar = stream->codecpar;

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        throw std::runtime_error(
            "Unsupported codec (ID: " + std::to_string(codecpar->codec_id) + ") in: " + path_);
    }

    AVCodecContext* raw_codec_ctx = avcodec_alloc_context3(codec);
    if (!raw_codec_ctx) {
        throw std::runtime_error("Failed to allocate decoder context for: " + path_);
    }
    codec_ctx_.reset(raw_codec_ctx);

    ret = avcodec_parameters_to_context(codec_ctx_.get(), codecpar);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(
            "Failed to copy codec parameters: " + std::string(errbuf));
    }

    ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        throw std::runtime_error(
            "Failed to open decoder: " + std::string(errbuf));
    }

    // Calculate duration
    if (fmt_ctx_->duration > 0) {
        duration_ = static_cast<double>(fmt_ctx_->duration) / AV_TIME_BASE;
    } else if (stream->duration > 0) {
        duration_ = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    } else {
        duration_ = 0.0;
    }

    orig_width_ = codecpar->width;
    orig_height_ = codecpar->height;

    // Calculate frame rate
    AVRational framerate = av_guess_frame_rate(fmt_ctx_.get(), stream, nullptr);
    if (framerate.num > 0 && framerate.den > 0) {
        fps_ = av_q2d(framerate);
    } else if (stream->avg_frame_rate.num > 0) {
        fps_ = av_q2d(stream->avg_frame_rate);
    } else if (stream->r_frame_rate.num > 0) {
        fps_ = av_q2d(stream->r_frame_rate);
    } else {
        constexpr double kDefaultFps = 30.0;
        fps_ = kDefaultFps;
    }

    // Allocate frame buffers
    AVFrame* raw_frame = av_frame_alloc();
    AVFrame* raw_rgb_frame = av_frame_alloc();
    if (!raw_frame || !raw_rgb_frame) {
        if (raw_frame) av_frame_free(&raw_frame);
        if (raw_rgb_frame) av_frame_free(&raw_rgb_frame);
        throw std::runtime_error("Failed to allocate frame buffers for: " + path_);
    }
    frame_.reset(raw_frame);
    rgb_frame_.reset(raw_rgb_frame);
}

void FFmpegDecoder::setup_scaler(int src_w, int src_h, int dst_w, int dst_h) {
    if (sws_ctx_ && scaler_dst_width_ == dst_w && scaler_dst_height_ == dst_h) {
        return;
    }
    sws_ctx_.reset(sws_getContext(
        src_w, src_h, codec_ctx_->pix_fmt,
        dst_w, dst_h, AV_PIX_FMT_RGBA,
        SWS_LANCZOS, nullptr, nullptr, nullptr));
    if (!sws_ctx_) throw std::runtime_error("Failed to create scaler");
    scaler_dst_width_ = dst_w;
    scaler_dst_height_ = dst_h;
}

void FFmpegDecoder::setup_rgb_buffer(int width, int height) {
    if (rgb_buffer_ && rgb_frame_->width == width && rgb_frame_->height == height) {
        return;
    }
    rgb_buffer_.reset(nullptr);
    int rgb_buf_size = av_image_get_buffer_size(
        AV_PIX_FMT_RGBA, width, height, 1);
    uint8_t* raw_rgb_buffer = static_cast<uint8_t*>(av_malloc(rgb_buf_size));
    if (!raw_rgb_buffer) throw std::runtime_error("Failed to allocate RGB buffer");
    rgb_buffer_.reset(raw_rgb_buffer);

    av_image_fill_arrays(
        rgb_frame_->data, rgb_frame_->linesize,
        rgb_buffer_.get(), AV_PIX_FMT_RGBA,
        width, height, 1);

    rgb_frame_->width = width;
    rgb_frame_->height = height;
    rgb_frame_->format = AV_PIX_FMT_RGBA;
}

std::optional<Frame> FFmpegDecoder::decode_next_frame() {
    AVPacket* raw_packet = av_packet_alloc();
    if (!raw_packet) throw std::runtime_error("Failed to allocate AVPacket");
    AVPacketGuard guard(raw_packet);
    AVPacket* packet = guard.packet;

    while (true) {
        int read_ret = av_read_frame(fmt_ctx_.get(), packet);
        if (read_ret >= 0) {
            if (packet->stream_index == video_stream_idx_) {
                int ret = avcodec_send_packet(codec_ctx_.get(), packet);
                av_packet_unref(packet);
                if (ret < 0) {
                    throw std::runtime_error("Failed to send packet");
                }

                ret = avcodec_receive_frame(codec_ctx_.get(), frame_.get());
                if (ret == AVERROR(EAGAIN)) {
                    continue;
                } else if (ret == AVERROR_EOF) {
                    return std::nullopt;
                } else if (ret < 0) {
                    throw std::runtime_error("Failed to receive frame");
                }

                return convert_frame();
            }
            av_packet_unref(packet);
        } else if (read_ret == AVERROR_EOF) {
            // File fully read, send nullptr packet to trigger decoder flush
            avcodec_send_packet(codec_ctx_.get(), nullptr);

            int ret = avcodec_receive_frame(codec_ctx_.get(), frame_.get());
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                return std::nullopt;
            } else if (ret < 0) {
                throw std::runtime_error("Failed to receive frame during flush");
            }

            return convert_frame();
        } else {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(read_ret, errbuf, sizeof(errbuf));
            throw std::runtime_error("Failed to read frame: " + std::string(errbuf));
        }
    }
}

Frame FFmpegDecoder::convert_frame() {
    if (!sws_ctx_) {
        throw std::runtime_error("Scaler not initialized");
    }
    sws_scale(sws_ctx_.get(),
        frame_->data, frame_->linesize,
        0, codec_ctx_->height,
        rgb_frame_->data, rgb_frame_->linesize);

    Frame result;
    result.width = rgb_frame_->width;
    result.height = rgb_frame_->height;
    // Use to_gif::kBytesPerPixel
    int row_bytes = result.width * kBytesPerPixel;
    result.rgba_pixels.resize(result.height * row_bytes);
    for (int y = 0; y < result.height; ++y) {
        std::memcpy(
            result.rgba_pixels.data() + y * row_bytes,
            rgb_buffer_.get() + y * rgb_frame_->linesize[0],
            row_bytes);
    }
    return result;
}

std::vector<Frame> FFmpegDecoder::extract_frames(
    double target_fps,
    int target_width,
    double start_sec,
    double end_sec) {

    if (target_fps <= 0 || target_width <= 0) {
        throw std::invalid_argument("Target fps and width must be greater than 0");
    }

    int target_height = calc_height(target_width, orig_width_, orig_height_);

    setup_scaler(orig_width_, orig_height_, target_width, target_height);
    setup_rgb_buffer(target_width, target_height);

    double start_pts = start_sec >= 0 ? start_sec : 0.0;
    double end_pts = end_sec >= 0 ? end_sec : duration_;

    // Unconditionally seek to start to ensure repeatable calls
    int64_t ts = static_cast<int64_t>(start_pts * AV_TIME_BASE);
    if (av_seek_frame(fmt_ctx_.get(), -1, ts, AVSEEK_FLAG_BACKWARD) >= 0) {
        avcodec_flush_buffers(codec_ctx_.get());
    }

    std::vector<Frame> frames;
    double frame_interval = 1.0 / target_fps;
    double next_pts = start_pts;

    while (true) {
        auto frame_opt = decode_next_frame();
        if (!frame_opt) break;
        Frame& frame = *frame_opt;

        int64_t timestamp = frame_->best_effort_timestamp != AV_NOPTS_VALUE
            ? frame_->best_effort_timestamp
            : frame_->pts;
        double current_pts = static_cast<double>(timestamp) *
            av_q2d(fmt_ctx_->streams[video_stream_idx_]->time_base);

        if (current_pts < start_pts) continue;
        if (current_pts > end_pts) break;

        if (current_pts >= next_pts) {
            frames.push_back(std::move(frame));
            next_pts += frame_interval;
        }
    }

    return frames;
}

double FFmpegDecoder::duration() const { return duration_; }
int    FFmpegDecoder::original_width() const { return orig_width_; }
int    FFmpegDecoder::original_height() const { return orig_height_; }
double FFmpegDecoder::original_fps() const { return fps_; }

} // namespace to_gif
