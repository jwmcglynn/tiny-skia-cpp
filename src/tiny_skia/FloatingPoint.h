#pragma once

#include <cstdint>
#include <optional>

namespace tiny_skia {

/// A float value in [0, 1].
/// Matches Rust `NormalizedF32`.
class NormalizedF32 {
 public:
  static const NormalizedF32 ZERO;
  static const NormalizedF32 ONE;

  constexpr NormalizedF32() = default;
  explicit constexpr NormalizedF32(float value) : value_(value) {}

  static constexpr NormalizedF32 newUnchecked(float value) { return NormalizedF32(value); }

  [[nodiscard]] static std::optional<NormalizedF32> create(float value) { return newFloat(value); }

  [[nodiscard]] static std::optional<NormalizedF32> newFloat(float value);
  [[nodiscard]] static NormalizedF32 newClamped(float value);
  [[nodiscard]] static NormalizedF32 fromU8(std::uint8_t value);

  [[nodiscard]] constexpr float get() const { return value_; }

  constexpr bool operator==(const NormalizedF32&) const = default;
  constexpr bool operator<(const NormalizedF32& o) const { return value_ < o.value_; }
  constexpr bool operator<=(const NormalizedF32& o) const { return value_ <= o.value_; }
  constexpr bool operator>(const NormalizedF32& o) const { return value_ > o.value_; }
  constexpr bool operator>=(const NormalizedF32& o) const { return value_ >= o.value_; }

  [[nodiscard]] static constexpr NormalizedF32 zero() { return NormalizedF32(0.0f); }
  [[nodiscard]] static constexpr NormalizedF32 one() { return NormalizedF32(1.0f); }

 private:
  float value_ = 0.0f;
};

inline constexpr NormalizedF32 NormalizedF32::ZERO = NormalizedF32(0.0f);
inline constexpr NormalizedF32 NormalizedF32::ONE = NormalizedF32(1.0f);

/// A float value in (0, 1) exclusive.
/// Matches Rust `NormalizedF32Exclusive`.
class NormalizedF32Exclusive {
 public:
  static const NormalizedF32Exclusive ANY;
  static const NormalizedF32Exclusive HALF;

  [[nodiscard]] static std::optional<NormalizedF32Exclusive> create(float v);
  [[nodiscard]] static NormalizedF32Exclusive newBounded(float v);

  [[nodiscard]] constexpr float get() const { return value_; }
  [[nodiscard]] NormalizedF32 toNormalized() const;

  constexpr bool operator==(const NormalizedF32Exclusive&) const = default;
  constexpr bool operator<(const NormalizedF32Exclusive& o) const { return value_ < o.value_; }

 private:
  explicit constexpr NormalizedF32Exclusive(float v) : value_(v) {}
  float value_ = 0.5f;
};

inline constexpr NormalizedF32Exclusive NormalizedF32Exclusive::ANY = NormalizedF32Exclusive(0.5f);
inline constexpr NormalizedF32Exclusive NormalizedF32Exclusive::HALF = NormalizedF32Exclusive(0.5f);

/// A float value guaranteed to be > 0 and finite.
/// Matches Rust `NonZeroPositiveF32`.
class NonZeroPositiveF32 {
 public:
  [[nodiscard]] static std::optional<NonZeroPositiveF32> create(float v);
  [[nodiscard]] constexpr float get() const { return value_; }

 private:
  explicit constexpr NonZeroPositiveF32(float v) : value_(v) {}
  float value_ = 1.0f;
};

/// A float value guaranteed to be finite.
/// Matches Rust `FiniteF32`.
class FiniteF32 {
 public:
  [[nodiscard]] static std::optional<FiniteF32> create(float v);
  [[nodiscard]] constexpr float get() const { return value_; }

 private:
  explicit constexpr FiniteF32(float v) : value_(v) {}
  float value_ = 0.0f;
};

/// Returns the closest `int32_t` for the given float, clamping into
/// [`-2147483520.0f`, `2147483520.0f`].
[[nodiscard]] std::int32_t saturateCastI32(float x);

/// Returns the closest `int32_t` for the given double, clamping into
/// [`INT32_MIN`, `INT32_MAX`].
[[nodiscard]] std::int32_t saturateCastI32(double x);

/// Saturating floor conversion from float to `int32_t`.
[[nodiscard]] std::int32_t saturateFloorToI32(float x);

/// Saturating ceil conversion from float to `int32_t`.
[[nodiscard]] std::int32_t saturateCeilToI32(float x);

/// Saturating round conversion from float to `int32_t`.
/// Matches Rust `SaturateRound<f32>::saturate_round`.
[[nodiscard]] std::int32_t saturateRoundToI32(float x);

/// Converts a float bit pattern to a comparable two's-complement int ordering.
/// Matches Rust `f32_as_2s_compliment`.
[[nodiscard]] std::int32_t f32As2sCompliment(float x);

}  // namespace tiny_skia
