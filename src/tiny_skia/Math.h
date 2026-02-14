#pragma once

#include <algorithm>
#include <cstdint>

namespace tiny_skia {

using LengthU32 = std::uint32_t;

constexpr LengthU32 kLengthU32One = 1;

template <typename T>
constexpr T bound(T min, T value, T max) {
  return std::min(max, std::max(min, value));
}

int leftShift(int32_t value, int32_t shift);
long long leftShift64(long long value, int32_t shift);
float approxPowf(float x, float y);

}  // namespace tiny_skia
