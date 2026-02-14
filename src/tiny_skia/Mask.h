#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Pixmap.h"

namespace tiny_skia {

enum class MaskType : std::uint8_t {
  Alpha = 0,
  Luminance = 1,
};

struct SubMaskRef {
  IntSize size{};
  std::uint32_t realWidth = 0;
  const std::uint8_t* data = nullptr;
};

struct SubMaskMut {
  IntSize size{};
  std::uint32_t realWidth = 0;
  std::uint8_t* data = nullptr;
};

class Mask {
 public:
  Mask() = default;

  static std::optional<Mask> fromSize(std::uint32_t width, std::uint32_t height);
  static std::optional<Mask> fromVec(std::vector<std::uint8_t> data, IntSize size);
  static Mask fromPixmap(const PixmapRef& pixmap, MaskType maskType);

  [[nodiscard]] std::uint32_t width() const {
    return size_.width();
  }

  [[nodiscard]] std::uint32_t height() const {
    return size_.height();
  }

  [[nodiscard]] IntSize size() const {
    return size_;
  }

  [[nodiscard]] std::span<const std::uint8_t> data() const {
    return std::span<const std::uint8_t>(data_.data(), data_.size());
  }

  [[nodiscard]] std::span<std::uint8_t> dataMut() {
    return std::span<std::uint8_t>(data_.data(), data_.size());
  }

  [[nodiscard]] SubMaskRef asSubmask() const;
  [[nodiscard]] std::optional<SubMaskRef> submask(IntRect rect) const;

  [[nodiscard]] SubMaskMut asSubpixmap();
  [[nodiscard]] std::optional<SubMaskMut> subpixmap(IntRect rect);

  std::vector<std::uint8_t> take();

 private:
  explicit Mask(std::vector<std::uint8_t> data, IntSize size)
      : data_(std::move(data)), size_(size) {}

  std::vector<std::uint8_t> data_;
  IntSize size_;
};

}  // namespace tiny_skia
