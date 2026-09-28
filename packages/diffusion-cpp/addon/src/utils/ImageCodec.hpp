#pragma once

#include <cstdint>
#include <vector>

#include <stable-diffusion.h>

namespace image_codec {

inline constexpr uint64_t MAX_DECODED_PIXELS = 64ULL * 1024 * 1024;
inline constexpr uint64_t MAX_JOB_DECODED_PIXELS = 128ULL * 1024 * 1024;

struct FreeDeleter {
  void operator()(uint8_t* ptr) const noexcept;
};

std::vector<uint8_t> encodeToPng(const sd_image_t& image);
// Lossy JPEG encode (quality 1..100). Returns empty vector on failure.
// JPEG has no alpha: channel must be 1 or 3.
std::vector<uint8_t> encodeToJpeg(const sd_image_t& image, int quality);
sd_image_t decodeImage(
    const std::vector<uint8_t>& imageBytes,
    uint64_t pixelLimit = MAX_DECODED_PIXELS);

} // namespace image_codec
