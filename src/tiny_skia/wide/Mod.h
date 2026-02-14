#pragma once

#include <type_traits>

namespace tiny_skia::wide {

template <typename T>
[[nodiscard]] constexpr T genericBitBlend(T mask, T y, T n) {
  static_assert(std::is_integral_v<T>, "genericBitBlend requires integral type");
  return n ^ ((n ^ y) & mask);
}

[[nodiscard]] float fasterMin(float lhs, float rhs);
[[nodiscard]] float fasterMax(float lhs, float rhs);

}  // namespace tiny_skia::wide
