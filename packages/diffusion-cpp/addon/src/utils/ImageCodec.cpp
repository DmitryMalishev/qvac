#include "ImageCodec.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <memory>

// clang-analyzer reports false-positive leaks inside STB implementation paths
// that are not owned by this wrapper. Normal builds still compile the
// implementation here; analyzer runs only need the declarations.
#define STBI_NO_HDR
#define STBI_NO_TGA
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_GIF
#define STBI_NO_BMP
#define STBI_MAX_DIMENSIONS 16384

#if defined(__clang_analyzer__)
#include <stb_image.h>
#include <stb_image_write.h>
#else
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

namespace image_codec {

namespace {

// STB requires this exact C callback shape for stbi_write_*_to_func.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void writePngBytes(void* context, void* payload, int payloadSize) {
  if (context == nullptr || payload == nullptr || payloadSize <= 0) {
    return;
  }

  auto* output = static_cast<std::vector<uint8_t>*>(context);
  const auto* bytes = static_cast<const uint8_t*>(payload);
  std::copy_n(
      bytes,
      static_cast<std::size_t>(payloadSize),
      std::back_inserter(*output));
}

} // namespace

void FreeDeleter::operator()(uint8_t* ptr) const noexcept {
  if (ptr != nullptr) {
    stbi_image_free(ptr);
  }
}

std::vector<uint8_t> encodeToPng(const sd_image_t& image) {
  std::vector<uint8_t> out;
  const auto [width, height, channel, data] = image;
  if (data == nullptr || width == 0 || height == 0 || channel == 0 ||
      channel > 4) {
    return out;
  }
  if (width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    return out;
  }

  const uint64_t stride =
      static_cast<uint64_t>(width) * static_cast<uint64_t>(channel);
  if (stride > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
    return out;
  }

  const int writeResult = stbi_write_png_to_func(
      writePngBytes,
      &out,
      static_cast<int>(width),
      static_cast<int>(height),
      static_cast<int>(channel),
      data,
      static_cast<int>(stride));
  if (writeResult == 0) {
    out.clear();
  }
  return out;
}

std::vector<uint8_t> encodeToJpeg(const sd_image_t& image, int quality) {
  std::vector<uint8_t> out;
  const auto [width, height, channel, data] = image;
  if (data == nullptr || width == 0 || height == 0 ||
      (channel != 1 && channel != 3) || quality < 1 || quality > 100) {
    return out;
  }
  if (width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
      height > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
    return out;
  }

  const int writeResult = stbi_write_jpg_to_func(
      writePngBytes,
      &out,
      static_cast<int>(width),
      static_cast<int>(height),
      static_cast<int>(channel),
      data,
      quality);
  if (writeResult == 0) {
    out.clear();
  }
  return out;
}

sd_image_t
decodeImage(const std::vector<uint8_t>& imageBytes, uint64_t pixelLimit) {
  constexpr size_t maxCompressedBytes = 50ULL * 1024 * 1024;
  constexpr std::array<uint8_t, 8> pngSignature = {
      0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  const bool isPng =
      imageBytes.size() >= pngSignature.size() &&
      std::equal(pngSignature.begin(), pngSignature.end(), imageBytes.begin());
  const bool isJpeg = imageBytes.size() >= 3 && imageBytes[0] == 0xFF &&
                      imageBytes[1] == 0xD8 && imageBytes[2] == 0xFF;
  if ((!isPng && !isJpeg) || imageBytes.size() > maxCompressedBytes ||
      pixelLimit == 0 ||
      imageBytes.size() >
          static_cast<size_t>(std::numeric_limits<int>::max())) {
    return sd_image_t{};
  }

  int decodedWidth = 0;
  int decodedHeight = 0;
  int sourceChannels = 0;
  constexpr int desiredChannels = 3;
  const int inputSize = static_cast<int>(imageBytes.size());
  if (stbi_info_from_memory(
          imageBytes.data(),
          inputSize,
          &decodedWidth,
          &decodedHeight,
          &sourceChannels) == 0 ||
      decodedWidth <= 0 || decodedHeight <= 0 ||
      decodedWidth > STBI_MAX_DIMENSIONS ||
      decodedHeight > STBI_MAX_DIMENSIONS ||
      static_cast<uint64_t>(decodedWidth) *
              static_cast<uint64_t>(decodedHeight) >
          std::min(pixelLimit, MAX_DECODED_PIXELS)) {
    return sd_image_t{};
  }

  int loadedWidth = 0;
  int loadedHeight = 0;
  // clang-analyzer can miss that decodedData owns and releases STB memory.
  // NOLINTNEXTLINE(clang-analyzer-unix.Malloc)
  std::unique_ptr<uint8_t, FreeDeleter> decodedData(stbi_load_from_memory(
      imageBytes.data(),
      inputSize,
      &loadedWidth,
      &loadedHeight,
      &sourceChannels,
      desiredChannels));
  if (decodedData == nullptr || loadedWidth != decodedWidth ||
      loadedHeight != decodedHeight) {
    return sd_image_t{};
  }

  return sd_image_t{
      static_cast<uint32_t>(decodedWidth),
      static_cast<uint32_t>(decodedHeight),
      static_cast<uint32_t>(desiredChannels),
      decodedData.release()};
}

} // namespace image_codec
