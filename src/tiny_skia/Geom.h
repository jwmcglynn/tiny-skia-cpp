#pragma once

#include <cstdint>
#include <optional>

#include "tiny_skia/Math.h"

namespace tiny_skia {

class ScreenIntRect;
class IntRect;
class Rect;

class IntSize {
 public:
  constexpr IntSize() = default;
  constexpr IntSize(LengthU32 width, LengthU32 height) : width_(width), height_(height) {}

  static std::optional<IntSize> fromWh(LengthU32 width, LengthU32 height) {
    if (width == 0u || height == 0u) {
      return std::nullopt;
    }
    return IntSize{width, height};
  }

  constexpr LengthU32 width() const {
    return width_;
  }
  constexpr LengthU32 height() const {
    return height_;
  }
  [[nodiscard]] ScreenIntRect toScreenIntRect(std::uint32_t x, std::uint32_t y) const;

  constexpr bool operator==(const IntSize&) const = default;

 private:
  LengthU32 width_ = 0;
  LengthU32 height_ = 0;
};

class ScreenIntRect {
 public:
  constexpr ScreenIntRect() = default;

  static std::optional<ScreenIntRect> fromXYWH(std::uint32_t x,
                                               std::uint32_t y,
                                               std::uint32_t width,
                                               std::uint32_t height);

  static constexpr ScreenIntRect fromXYWHSafe(std::uint32_t x,
                                              std::uint32_t y,
                                              LengthU32 width,
                                              LengthU32 height) {
    return ScreenIntRect{x, y, width, height};
  }

  constexpr std::uint32_t x() const {
    return x_;
  }
  constexpr std::uint32_t y() const {
    return y_;
  }
  constexpr LengthU32 width() const {
    return width_;
  }
  constexpr LengthU32 height() const {
    return height_;
  }
  constexpr LengthU32 widthSafe() const {
    return width_;
  }

  constexpr std::uint32_t left() const {
    return x_;
  }
  constexpr std::uint32_t top() const {
    return y_;
  }

  std::uint32_t right() const;
  std::uint32_t bottom() const;
  IntSize size() const;
  [[nodiscard]] bool contains(const ScreenIntRect& other) const;
  [[nodiscard]] IntRect toIntRect() const;
  [[nodiscard]] Rect toRect() const;

 private:
  constexpr ScreenIntRect(std::uint32_t x,
                          std::uint32_t y,
                          LengthU32 width,
                          LengthU32 height)
      : x_{x}, y_{y}, width_{width}, height_{height} {}

  std::uint32_t x_ = 0;
  std::uint32_t y_ = 0;
  LengthU32 width_;
  LengthU32 height_;
};

class IntRect {
 public:
  static std::optional<IntRect> fromXYWH(std::int32_t x,
                                         std::int32_t y,
                                         std::uint32_t width,
                                         std::uint32_t height);

  constexpr std::int32_t x() const {
    return x_;
  }
  constexpr std::int32_t y() const {
    return y_;
  }
  constexpr LengthU32 width() const {
    return width_;
  }
  constexpr LengthU32 height() const {
    return height_;
  }

  [[nodiscard]] constexpr std::int32_t left() const {
    return x_;
  }
  [[nodiscard]] constexpr std::int32_t top() const {
    return y_;
  }

  [[nodiscard]] std::int32_t right() const;
  [[nodiscard]] std::int32_t bottom() const;

  [[nodiscard]] std::optional<IntRect> intersect(const IntRect& other) const;

  [[nodiscard]] std::optional<ScreenIntRect> toScreenIntRect() const;

 private:
  constexpr IntRect(std::int32_t x,
                    std::int32_t y,
                    LengthU32 width,
                    LengthU32 height)
      : x_{x}, y_{y}, width_{width}, height_{height} {}

  std::int32_t x_ = 0;
  std::int32_t y_ = 0;
  LengthU32 width_ = 0;
  LengthU32 height_ = 0;
};

class Rect {
 public:
  static std::optional<Rect> fromLtrb(float left, float top, float right, float bottom);

  constexpr float left() const {
    return left_;
  }
  constexpr float top() const {
    return top_;
  }
  constexpr float right() const {
    return right_;
  }
  constexpr float bottom() const {
    return bottom_;
  }

  constexpr bool operator==(const Rect&) const = default;

  [[nodiscard]] std::optional<IntRect> roundOut() const;

 private:
  constexpr Rect(float left, float top, float right, float bottom)
      : left_(left), top_(top), right_(right), bottom_(bottom) {}

  float left_ = 0.0f;
  float top_ = 0.0f;
  float right_ = 0.0f;
  float bottom_ = 0.0f;
};

std::optional<ScreenIntRect> intRectToScreen(const IntRect& rect);

}  // namespace tiny_skia
