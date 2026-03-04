#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"

namespace tiny_skia {

inline constexpr std::size_t kBytesPerPixel = 4;

class Pixmap;
struct SubPixmapMut;

class PixmapRef {
 public:
  PixmapRef() = default;

  static std::optional<PixmapRef> fromBytes(std::span<const std::uint8_t> data, std::uint32_t width,
                                            std::uint32_t height);

  [[nodiscard]] std::uint32_t width() const { return size_.width(); }

  [[nodiscard]] std::uint32_t height() const { return size_.height(); }

  [[nodiscard]] IntSize size() const { return size_; }

  [[nodiscard]] std::span<const std::uint8_t> data() const {
    return std::span<const std::uint8_t>(data_, len_);
  }

  [[nodiscard]] std::span<const PremultipliedColorU8> pixels() const;

  [[nodiscard]] std::optional<PremultipliedColorU8> pixel(std::uint32_t x, std::uint32_t y) const;
  [[nodiscard]] std::optional<Pixmap> cloneRect(const IntRect& rect) const;

 private:
  friend class Pixmap;

  explicit PixmapRef(const std::uint8_t* data, std::size_t len, IntSize size)
      : data_(data), len_(len), size_(size) {}

  const std::uint8_t* data_ = nullptr;
  std::size_t len_ = 0;
  IntSize size_;
};

class PixmapMut {
 public:
  PixmapMut() = default;
  explicit PixmapMut(std::uint8_t* data, std::size_t len, IntSize size)
      : data_(data), len_(len), size_(size) {}

  static std::optional<PixmapMut> fromBytes(std::span<std::uint8_t> data, std::uint32_t width,
                                            std::uint32_t height);

  [[nodiscard]] std::uint32_t width() const { return size_.width(); }

  [[nodiscard]] std::uint32_t height() const { return size_.height(); }

  [[nodiscard]] IntSize size() const { return size_; }

  [[nodiscard]] std::span<std::uint8_t> dataMut() const {
    return std::span<std::uint8_t>(data_, len_);
  }

  [[nodiscard]] std::span<PremultipliedColorU8> pixelsMut() const;
  [[nodiscard]] SubPixmapMut asSubpixmap() const;
  [[nodiscard]] std::optional<SubPixmapMut> subpixmap(const IntRect& rect) const;

 private:
  std::uint8_t* data_ = nullptr;
  std::size_t len_ = 0;
  IntSize size_;
};

struct SubPixmapMut {
  IntSize size{};
  std::size_t realWidth = 0;
  std::uint8_t* data = nullptr;

  [[nodiscard]] std::size_t width() const { return size.width(); }

  [[nodiscard]] std::size_t height() const { return size.height(); }

  [[nodiscard]] std::span<std::uint8_t> dataMut() const;
};

class Pixmap {
 public:
  Pixmap() = default;

  static std::optional<Pixmap> fromSize(std::uint32_t width, std::uint32_t height);
  static std::optional<Pixmap> fromVec(std::vector<std::uint8_t> data, IntSize size);

  [[nodiscard]] PixmapRef asRef() const { return PixmapRef(data_.data(), data_.size(), size_); }

  [[nodiscard]] PixmapMut asMut() { return PixmapMut(data_.data(), data_.size(), size_); }

  [[nodiscard]] std::uint32_t width() const { return size_.width(); }

  [[nodiscard]] std::uint32_t height() const { return size_.height(); }

  [[nodiscard]] IntSize size() const { return size_; }

  [[nodiscard]] std::span<const std::uint8_t> data() const {
    return std::span<const std::uint8_t>(data_.data(), data_.size());
  }

  [[nodiscard]] std::span<std::uint8_t> dataMut() {
    return std::span<std::uint8_t>(data_.data(), data_.size());
  }

  [[nodiscard]] std::span<const PremultipliedColorU8> pixels() const;
  [[nodiscard]] std::span<PremultipliedColorU8> pixelsMut();

  [[nodiscard]] std::optional<PremultipliedColorU8> pixel(std::uint32_t x, std::uint32_t y) const;
  [[nodiscard]] std::optional<Pixmap> cloneRect(const IntRect& rect) const;

  void fill(const Color& color);

  [[nodiscard]] std::vector<std::uint8_t> take();
  [[nodiscard]] std::vector<std::uint8_t> takeDemultiplied();

 private:
  explicit Pixmap(std::vector<std::uint8_t> data, IntSize size)
      : data_(std::move(data)), size_(size) {}

  std::vector<std::uint8_t> data_;
  IntSize size_;
};

}  // namespace tiny_skia
