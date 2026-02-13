#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "tiny_skia/Math.h"

namespace tiny_skia {

using AlphaU8 = std::uint8_t;

constexpr AlphaU8 kAlphaU8Transparent = 0x00;
constexpr AlphaU8 kAlphaU8Opaque = 0xFF;

namespace pipeline {

enum class Stage : std::uint8_t {
  GammaExpand2,
  GammaExpandDestination2,
  GammaExpand22,
  GammaExpandDestination22,
  GammaExpandSrgb,
  GammaExpandDestinationSrgb,
  GammaCompress2,
  GammaCompress22,
  GammaCompressSrgb,
};

}  // namespace pipeline

class NormalizedF32 {
 public:
  constexpr NormalizedF32() = default;
  explicit constexpr NormalizedF32(float value) : value_(value) {}

  static constexpr NormalizedF32 newUnchecked(float value) {
    return NormalizedF32(value);
  }
  static std::optional<NormalizedF32> newFloat(float value);
  static NormalizedF32 newClamped(float value);
  static NormalizedF32 fromU8(AlphaU8 value);

  constexpr float get() const {
    return value_;
  }
  constexpr bool operator==(const NormalizedF32&) const = default;

  static constexpr NormalizedF32 zero() {
    return NormalizedF32(0.0f);
  }
  static constexpr NormalizedF32 one() {
    return NormalizedF32(1.0f);
  }

 private:
  float value_ = 0.0f;
};

class Color;
class PremultipliedColor;
class PremultipliedColorU8;

class ColorU8 {
 public:
  constexpr ColorU8() = default;
  constexpr ColorU8(AlphaU8 red, AlphaU8 green, AlphaU8 blue, AlphaU8 alpha)
      : data_{red, green, blue, alpha} {}

  static constexpr ColorU8 fromRgba(AlphaU8 red,
                                    AlphaU8 green,
                                    AlphaU8 blue,
                                    AlphaU8 alpha) {
    return ColorU8(red, green, blue, alpha);
  }

  constexpr AlphaU8 red() const {
    return data_[0];
  }
  constexpr AlphaU8 green() const {
    return data_[1];
  }
  constexpr AlphaU8 blue() const {
    return data_[2];
  }
  constexpr AlphaU8 alpha() const {
    return data_[3];
  }

  [[nodiscard]] bool isOpaque() const {
    return alpha() == kAlphaU8Opaque;
  }

  [[nodiscard]] PremultipliedColorU8 premultiply() const;

  bool operator==(const ColorU8&) const = default;

 private:
  std::array<AlphaU8, 4> data_{};
};

class PremultipliedColorU8 {
 public:
  static const PremultipliedColorU8 transparent;
  constexpr PremultipliedColorU8() = default;

  static std::optional<PremultipliedColorU8> fromRgba(AlphaU8 red,
                                                     AlphaU8 green,
                                                     AlphaU8 blue,
                                                     AlphaU8 alpha);
  static constexpr PremultipliedColorU8 fromRgbaUnchecked(AlphaU8 red,
                                                          AlphaU8 green,
                                                          AlphaU8 blue,
                                                          AlphaU8 alpha) {
    return PremultipliedColorU8(red, green, blue, alpha);
  }

  constexpr PremultipliedColorU8(AlphaU8 red, AlphaU8 green, AlphaU8 blue, AlphaU8 alpha)
      : data_{red, green, blue, alpha} {}

  constexpr AlphaU8 red() const {
    return data_[0];
  }
  constexpr AlphaU8 green() const {
    return data_[1];
  }
  constexpr AlphaU8 blue() const {
    return data_[2];
  }
  constexpr AlphaU8 alpha() const {
    return data_[3];
  }

  [[nodiscard]] bool isOpaque() const {
    return alpha() == kAlphaU8Opaque;
  }

  [[nodiscard]] ColorU8 demultiply() const;

  bool operator==(const PremultipliedColorU8&) const = default;

 private:
  std::array<AlphaU8, 4> data_{};
};

class Color {
 public:
  Color() = default;
  static const Color transparent;
  static const Color black;
  static const Color white;

  constexpr Color(NormalizedF32 red,
                  NormalizedF32 green,
                  NormalizedF32 blue,
                  NormalizedF32 alpha)
      : red_(red), green_(green), blue_(blue), alpha_(alpha) {}

  static Color fromRgbaUnchecked(float red, float green, float blue, float alpha);
  static std::optional<Color> fromRgba(float red, float green, float blue, float alpha);
  static Color fromRgba8(AlphaU8 red, AlphaU8 green, AlphaU8 blue, AlphaU8 alpha);

  float red() const {
    return red_.get();
  }
  float green() const {
    return green_.get();
  }
  float blue() const {
    return blue_.get();
  }
  float alpha() const {
    return alpha_.get();
  }

  void setRed(float value) {
    red_ = NormalizedF32::newClamped(value);
  }
  void setGreen(float value) {
    green_ = NormalizedF32::newClamped(value);
  }
  void setBlue(float value) {
    blue_ = NormalizedF32::newClamped(value);
  }
  void setAlpha(float value) {
    alpha_ = NormalizedF32::newClamped(value);
  }

  void applyOpacity(float opacity) {
    alpha_ = NormalizedF32::newClamped(alpha_.get() * bound(0.0f, opacity, 1.0f));
  }

  [[nodiscard]] bool isOpaque() const {
    return alpha_ == NormalizedF32::one();
  }

  [[nodiscard]] PremultipliedColor premultiply() const;
  [[nodiscard]] ColorU8 toColorU8() const;
  bool operator==(const Color&) const = default;

 private:
  NormalizedF32 red_ = NormalizedF32::zero();
  NormalizedF32 green_ = NormalizedF32::zero();
  NormalizedF32 blue_ = NormalizedF32::zero();
  NormalizedF32 alpha_ = NormalizedF32::zero();
};

class PremultipliedColor {
 public:
  constexpr PremultipliedColor() = default;
  constexpr PremultipliedColor(NormalizedF32 red,
                              NormalizedF32 green,
                              NormalizedF32 blue,
                              NormalizedF32 alpha)
      : red_(red), green_(green), blue_(blue), alpha_(alpha) {}

  float red() const {
    return red_.get();
  }
  float green() const {
    return green_.get();
  }
  float blue() const {
    return blue_.get();
  }
  float alpha() const {
    return alpha_.get();
  }

  [[nodiscard]] Color demultiply() const;
  [[nodiscard]] PremultipliedColorU8 toColorU8() const;

  bool operator==(const PremultipliedColor&) const = default;

 private:
  friend class Color;
  NormalizedF32 red_ = NormalizedF32::zero();
  NormalizedF32 green_ = NormalizedF32::zero();
  NormalizedF32 blue_ = NormalizedF32::zero();
  NormalizedF32 alpha_ = NormalizedF32::zero();
};

AlphaU8 premultiplyU8(AlphaU8 color, AlphaU8 alpha);

enum class ColorSpace {
  Linear,
  Gamma2,
  SimpleSRGB,
  FullSRGBGamma,
};

NormalizedF32 expandChannel(ColorSpace colorSpace, NormalizedF32 x);
Color expandColor(ColorSpace colorSpace, Color color);
NormalizedF32 compressChannel(ColorSpace colorSpace, NormalizedF32 x);

std::optional<pipeline::Stage> expandStage(ColorSpace colorSpace);
std::optional<pipeline::Stage> expandDestStage(ColorSpace colorSpace);
std::optional<pipeline::Stage> compressStage(ColorSpace colorSpace);

}  // namespace tiny_skia
