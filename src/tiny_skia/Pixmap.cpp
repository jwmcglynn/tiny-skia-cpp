#include "tiny_skia/Pixmap.h"

#include <limits>
#include <utility>

namespace tiny_skia {

namespace {

std::optional<std::size_t> dataLenForSize(IntSize size) {
  constexpr auto kMaxWidth = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max() / 4);

  const auto width = static_cast<std::uint32_t>(size.width());
  const auto height = static_cast<std::uint32_t>(size.height());
  if (width > kMaxWidth) {
    return std::nullopt;
  }

  const auto pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
  const auto bytes = pixels * static_cast<std::uint64_t>(kBytesPerPixel);
  if (bytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(bytes);
}

std::optional<std::size_t> pixelIndex(std::uint32_t width,
                                      std::uint32_t height,
                                      std::uint32_t x,
                                      std::uint32_t y) {
  if (x >= width || y >= height) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

}  // namespace

std::optional<PixmapRef> PixmapRef::fromBytes(std::span<const std::uint8_t> data,
                                              std::uint32_t width,
                                              std::uint32_t height) {
  const auto size = IntSize::fromWh(width, height);
  if (!size.has_value()) {
    return std::nullopt;
  }

  const auto required = dataLenForSize(size.value());
  if (!required.has_value() || data.size() < required.value()) {
    return std::nullopt;
  }

  return PixmapRef(data.data(), data.size(), size.value());
}

std::span<const PremultipliedColorU8> PixmapRef::pixels() const {
  static_assert(sizeof(PremultipliedColorU8) == kBytesPerPixel);
  const auto* ptr = reinterpret_cast<const PremultipliedColorU8*>(data_);
  return std::span<const PremultipliedColorU8>(ptr, len_ / kBytesPerPixel);
}

std::optional<PremultipliedColorU8> PixmapRef::pixel(std::uint32_t x, std::uint32_t y) const {
  const auto index = pixelIndex(width(), height(), x, y);
  if (!index.has_value()) {
    return std::nullopt;
  }
  return pixels()[index.value()];
}

std::span<PremultipliedColorU8> PixmapMut::pixelsMut() const {
  static_assert(sizeof(PremultipliedColorU8) == kBytesPerPixel);
  auto* ptr = reinterpret_cast<PremultipliedColorU8*>(data_);
  return std::span<PremultipliedColorU8>(ptr, len_ / kBytesPerPixel);
}

std::span<std::uint8_t> SubPixmapMut::dataMut() const {
  const auto len =
      static_cast<std::size_t>(real_width) * static_cast<std::size_t>(size.height()) * kBytesPerPixel;
  return std::span<std::uint8_t>(data, len);
}

std::optional<Pixmap> Pixmap::fromSize(std::uint32_t width, std::uint32_t height) {
  const auto size = IntSize::fromWh(width, height);
  if (!size.has_value()) {
    return std::nullopt;
  }

  const auto len = dataLenForSize(size.value());
  if (!len.has_value()) {
    return std::nullopt;
  }

  return Pixmap(std::vector<std::uint8_t>(len.value(), 0), size.value());
}

std::optional<Pixmap> Pixmap::fromVec(std::vector<std::uint8_t> data, IntSize size) {
  const auto len = dataLenForSize(size);
  if (!len.has_value() || data.size() != len.value()) {
    return std::nullopt;
  }
  return Pixmap(std::move(data), size);
}

std::span<const PremultipliedColorU8> Pixmap::pixels() const {
  return asRef().pixels();
}

std::span<PremultipliedColorU8> Pixmap::pixelsMut() {
  return asMut().pixelsMut();
}

std::optional<PremultipliedColorU8> Pixmap::pixel(std::uint32_t x, std::uint32_t y) const {
  return asRef().pixel(x, y);
}

std::vector<std::uint8_t> Pixmap::take() {
  size_ = IntSize{};
  return std::move(data_);
}

}  // namespace tiny_skia
