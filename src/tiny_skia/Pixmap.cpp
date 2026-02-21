#include "tiny_skia/Pixmap.h"

#include <algorithm>
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

bool containsRect(const IntRect& outer, const IntRect& inner) {
  return outer.left() <= inner.left() && outer.top() <= inner.top() &&
         outer.right() >= inner.right() && outer.bottom() >= inner.bottom();
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

std::optional<Pixmap> PixmapRef::cloneRect(const IntRect& rect) const {
  const auto full = IntRect::fromXYWH(0, 0, width(), height());
  if (!full.has_value()) {
    return std::nullopt;
  }

  const auto clipped = full->intersect(rect);
  if (!clipped.has_value()) {
    return std::nullopt;
  }

  auto out = Pixmap::fromSize(clipped->width(), clipped->height());
  if (!out.has_value()) {
    return std::nullopt;
  }

  const auto srcBytes = data();
  auto dstBytes = out->dataMut();

  const auto srcWidth = static_cast<std::size_t>(width());
  const auto rowBytes = static_cast<std::size_t>(clipped->width()) * kBytesPerPixel;
  for (std::uint32_t y = 0; y < clipped->height(); ++y) {
    const auto srcOffset = (static_cast<std::size_t>(clipped->top()) + y) * srcWidth * kBytesPerPixel +
                           static_cast<std::size_t>(clipped->left()) * kBytesPerPixel;
    const auto dstOffset = static_cast<std::size_t>(y) * rowBytes;
    std::copy_n(srcBytes.data() + srcOffset, rowBytes, dstBytes.data() + dstOffset);
  }

  return out;
}

std::optional<PixmapMut> PixmapMut::fromBytes(std::span<std::uint8_t> data,
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

  return PixmapMut(data.data(), data.size(), size.value());
}

std::span<PremultipliedColorU8> PixmapMut::pixelsMut() const {
  static_assert(sizeof(PremultipliedColorU8) == kBytesPerPixel);
  auto* ptr = reinterpret_cast<PremultipliedColorU8*>(data_);
  return std::span<PremultipliedColorU8>(ptr, len_ / kBytesPerPixel);
}

SubPixmapMut PixmapMut::asSubpixmap() const {
  return SubPixmapMut{
      .size = size_,
      .real_width = static_cast<std::size_t>(size_.width()),
      .data = data_,
  };
}

std::optional<SubPixmapMut> PixmapMut::subpixmap(const IntRect& rect) const {
  const auto full = IntRect::fromXYWH(0, 0, width(), height());
  if (!full.has_value()) {
    return std::nullopt;
  }

  const auto intersection = full->intersect(rect);
  if (!intersection.has_value()) {
    return std::nullopt;
  }

  const auto srcWidth = static_cast<std::size_t>(size_.width());
  const auto offset =
      (static_cast<std::size_t>(intersection->top()) * srcWidth + static_cast<std::size_t>(intersection->left())) *
      kBytesPerPixel;
  const auto subSize = IntSize::fromWh(intersection->width(), intersection->height());
  if (!subSize.has_value()) {
    return std::nullopt;
  }

  return SubPixmapMut{
      .size = subSize.value(),
      .real_width = srcWidth,
      .data = data_ + offset,
  };
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

std::optional<Pixmap> Pixmap::cloneRect(const IntRect& rect) const {
  return asRef().cloneRect(rect);
}

void Pixmap::fill(const Color& color) {
  const auto c = color.premultiply().toColorU8();
  auto px = pixelsMut();
  for (auto& p : px) {
    p = c;
  }
}

std::vector<std::uint8_t> Pixmap::take() {
  size_ = IntSize{};
  return std::move(data_);
}

std::vector<std::uint8_t> Pixmap::takeDemultiplied() {
  auto px = pixelsMut();
  for (auto& p : px) {
    const auto c = p.demultiply();
    p = PremultipliedColorU8::fromRgbaUnchecked(c.red(), c.green(), c.blue(), c.alpha());
  }
  return take();
}

}  // namespace tiny_skia
