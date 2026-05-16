#include "components/giflib_encoder.hpp"
#include <array>
#include <cstring>
#include <stdexcept>

namespace to_gif {

namespace {

constexpr int kColorDepthBits = 8;
constexpr int kNetscapeIdLen = 11;
constexpr int kLoopExtBlockLen = 3;
constexpr int kGceExtSize = 4;

// giflib API lacks const qualifier, but EGifPutLine does not actually modify pixel data
GifPixelType* safe_pixel_ptr(const uint8_t* data) {
    return const_cast<GifPixelType*>(data);
}

} // namespace

GifLibEncoder::GifLibEncoder(const std::string& output_path) : path_(output_path) {}

GifLibEncoder::GifLibEncoder(GifLibEncoder&& other) noexcept
    : path_(std::move(other.path_)),
      gif_(other.gif_),
      cmap_(other.cmap_),
      begun_(other.begun_) {
    other.gif_ = nullptr;
    other.cmap_ = nullptr;
    other.begun_ = false;
}

GifLibEncoder& GifLibEncoder::operator=(GifLibEncoder&& other) noexcept {
    if (this != &other) {
        cleanup();
        path_ = std::move(other.path_);
        gif_ = other.gif_;
        cmap_ = other.cmap_;
        begun_ = other.begun_;
        other.gif_ = nullptr;
        other.cmap_ = nullptr;
        other.begun_ = false;
    }
    return *this;
}

void GifLibEncoder::cleanup() {
    if (gif_) {
        EGifCloseFile(gif_, nullptr);
        gif_ = nullptr;
    }
    if (cmap_) {
        GifFreeMapObject(cmap_);
        cmap_ = nullptr;
    }
}

GifLibEncoder::~GifLibEncoder() {
    cleanup();
}

void GifLibEncoder::begin(int width, int height, const std::vector<Color>& palette) {
    if (begun_) {
        throw std::runtime_error("GIF has already begun writing");
    }

    int error = 0;
    gif_ = EGifOpenFileName(path_.c_str(), false, &error);
    if (!gif_) {
        throw std::runtime_error("Failed to create GIF file: " + path_ +
            " (error=" + std::to_string(error) + ")");
    }

    int color_count = static_cast<int>(palette.size());
    cmap_ = GifMakeMapObject(color_count, nullptr);
    if (!cmap_) {
        throw std::runtime_error("Failed to create palette");
    }

    for (int i = 0; i < color_count; ++i) {
        cmap_->Colors[i].Red   = static_cast<GifByteType>(palette[i].r);
        cmap_->Colors[i].Green = static_cast<GifByteType>(palette[i].g);
        cmap_->Colors[i].Blue  = static_cast<GifByteType>(palette[i].b);
    }

    if (EGifPutScreenDesc(gif_, width, height, kColorDepthBits, 0, cmap_) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF screen descriptor");
    }

    // Infinite loop
    char loop_ext[] = {0x01, 0x00, 0x00};
    if (EGifPutExtensionLeader(gif_, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF extension leader");
    }
    if (EGifPutExtensionBlock(gif_, kNetscapeIdLen, (void*)"NETSCAPE2.0") == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF Netscape extension block");
    }
    if (EGifPutExtensionBlock(gif_, kLoopExtBlockLen, loop_ext) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF loop extension block");
    }
    if (EGifPutExtensionTrailer(gif_) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF extension trailer");
    }

    begun_ = true;
}

void GifLibEncoder::write_frame(
    const std::vector<uint8_t>& palette_indices,
    int width,
    int height,
    int delay_cs) {

    if (!begun_ || !gif_) {
        throw std::runtime_error("GIF has not begun writing");
    }

    GraphicsControlBlock gcb{};
    gcb.DisposalMode = DISPOSE_DO_NOT;
    gcb.DelayTime = delay_cs;
    gcb.TransparentColor = NO_TRANSPARENT_COLOR;

    std::array<uint8_t, kGceExtSize> ext_buffer;
    EGifGCBToExtension(&gcb, ext_buffer.data());
    if (EGifPutExtension(gif_, GRAPHICS_EXT_FUNC_CODE, kGceExtSize,
                         ext_buffer.data()) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF graphics control extension");
    }

    if (EGifPutImageDesc(gif_, 0, 0, width, height, false, nullptr) == GIF_ERROR) {
        throw std::runtime_error("Failed to write GIF image descriptor");
    }

    for (int y = 0; y < height; ++y) {
        if (EGifPutLine(gif_, safe_pixel_ptr(
            palette_indices.data() + y * width), width) == GIF_ERROR) {
            throw std::runtime_error("Failed to write GIF pixel row " + std::to_string(y));
        }
    }
}

void GifLibEncoder::end() {
    cleanup();
    begun_ = false;
}

} // namespace to_gif
