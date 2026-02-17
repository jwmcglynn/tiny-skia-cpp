#pragma once

#include <cmath>
#include <limits>
#include <optional>

#include "tiny_skia/Color.h"

namespace tiny_skia {

// NormalizedF32 is defined in Color.h.

/// A float value in (0, 1) exclusive.
/// Matches Rust `NormalizedF32Exclusive`.
class NormalizedF32Exclusive {
 public:
  static const NormalizedF32Exclusive HALF;

  [[nodiscard]] static std::optional<NormalizedF32Exclusive> create(float v) {
    if (v > 0.0f && v < 1.0f) {
      return NormalizedF32Exclusive(v);
    }
    return std::nullopt;
  }

  [[nodiscard]] static NormalizedF32Exclusive newBounded(float v) {
    float clamped = v;
    if (!(clamped > std::numeric_limits<float>::epsilon()))
      clamped = std::numeric_limits<float>::epsilon();
    if (clamped > 1.0f - std::numeric_limits<float>::epsilon())
      clamped = 1.0f - std::numeric_limits<float>::epsilon();
    return NormalizedF32Exclusive(clamped);
  }

  [[nodiscard]] constexpr float get() const { return value_; }

  [[nodiscard]] NormalizedF32 toNormalized() const {
    return NormalizedF32::newClamped(value_);
  }

  constexpr bool operator==(const NormalizedF32Exclusive& o) const {
    return value_ == o.value_;
  }
  constexpr bool operator<(const NormalizedF32Exclusive& o) const {
    return value_ < o.value_;
  }

 private:
  explicit constexpr NormalizedF32Exclusive(float v) : value_(v) {}
  float value_;
};

inline constexpr NormalizedF32Exclusive NormalizedF32Exclusive::HALF =
    NormalizedF32Exclusive(0.5f);

/// A float value guaranteed to be > 0.
/// Matches Rust `NonZeroPositiveF32`.
class NonZeroPositiveF32 {
 public:
  [[nodiscard]] static std::optional<NonZeroPositiveF32> create(float v) {
    if (v > 0.0f && std::isfinite(v)) {
      return NonZeroPositiveF32(v);
    }
    return std::nullopt;
  }

  [[nodiscard]] constexpr float get() const { return value_; }

 private:
  explicit constexpr NonZeroPositiveF32(float v) : value_(v) {}
  float value_;
};

/// A float value guaranteed to be finite.
/// Matches Rust `FiniteF32`.
class FiniteF32 {
 public:
  [[nodiscard]] static std::optional<FiniteF32> create(float v) {
    if (std::isfinite(v)) {
      return FiniteF32(v);
    }
    return std::nullopt;
  }

  [[nodiscard]] constexpr float get() const { return value_; }

 private:
  explicit constexpr FiniteF32(float v) : value_(v) {}
  float value_;
};

// --- Scalar trait utility functions (matches Rust Scalar trait on f32) ---

/// Returns x * 0.5.
[[nodiscard]] inline constexpr float scalarHalf(float x) {
  return x * 0.5f;
}

/// Returns the average of two values: (a + b) * 0.5.
[[nodiscard]] inline constexpr float scalarAve(float a, float b) {
  return (a + b) * 0.5f;
}

/// Returns x * x.
[[nodiscard]] inline constexpr float scalarSqr(float x) {
  return x * x;
}

/// Returns 1.0 / x.
[[nodiscard]] inline constexpr float scalarInvert(float x) {
  return 1.0f / x;
}

/// Non-panicking clamp: clamps x into [lo, hi].
[[nodiscard]] inline constexpr float scalarBound(float x, float lo,
                                                  float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

}  // namespace tiny_skia
