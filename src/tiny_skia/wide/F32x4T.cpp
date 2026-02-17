#include "tiny_skia/wide/F32x4T.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/I32x4T.h"
#include "tiny_skia/wide/Mod.h"

namespace tiny_skia::wide {

float F32x4T::cmpMask(bool predicate) {
  return predicate ? kTrueMask : 0.0f;
}

F32x4T F32x4T::abs() const {
  return F32x4T({std::fabs(lanes_[0]), std::fabs(lanes_[1]),
                 std::fabs(lanes_[2]), std::fabs(lanes_[3])});
}

F32x4T F32x4T::max(const F32x4T& rhs) const {
  return F32x4T({fasterMax(lanes_[0], rhs.lanes_[0]),
                 fasterMax(lanes_[1], rhs.lanes_[1]),
                 fasterMax(lanes_[2], rhs.lanes_[2]),
                 fasterMax(lanes_[3], rhs.lanes_[3])});
}

F32x4T F32x4T::min(const F32x4T& rhs) const {
  return F32x4T({fasterMin(lanes_[0], rhs.lanes_[0]),
                 fasterMin(lanes_[1], rhs.lanes_[1]),
                 fasterMin(lanes_[2], rhs.lanes_[2]),
                 fasterMin(lanes_[3], rhs.lanes_[3])});
}

F32x4T F32x4T::cmpEq(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] == rhs.lanes_[0]),
                 cmpMask(lanes_[1] == rhs.lanes_[1]),
                 cmpMask(lanes_[2] == rhs.lanes_[2]),
                 cmpMask(lanes_[3] == rhs.lanes_[3])});
}

F32x4T F32x4T::cmpNe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] != rhs.lanes_[0]),
                 cmpMask(lanes_[1] != rhs.lanes_[1]),
                 cmpMask(lanes_[2] != rhs.lanes_[2]),
                 cmpMask(lanes_[3] != rhs.lanes_[3])});
}

F32x4T F32x4T::cmpGe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] >= rhs.lanes_[0]),
                 cmpMask(lanes_[1] >= rhs.lanes_[1]),
                 cmpMask(lanes_[2] >= rhs.lanes_[2]),
                 cmpMask(lanes_[3] >= rhs.lanes_[3])});
}

F32x4T F32x4T::cmpGt(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] > rhs.lanes_[0]),
                 cmpMask(lanes_[1] > rhs.lanes_[1]),
                 cmpMask(lanes_[2] > rhs.lanes_[2]),
                 cmpMask(lanes_[3] > rhs.lanes_[3])});
}

F32x4T F32x4T::cmpLe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] <= rhs.lanes_[0]),
                 cmpMask(lanes_[1] <= rhs.lanes_[1]),
                 cmpMask(lanes_[2] <= rhs.lanes_[2]),
                 cmpMask(lanes_[3] <= rhs.lanes_[3])});
}

F32x4T F32x4T::cmpLt(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] < rhs.lanes_[0]),
                 cmpMask(lanes_[1] < rhs.lanes_[1]),
                 cmpMask(lanes_[2] < rhs.lanes_[2]),
                 cmpMask(lanes_[3] < rhs.lanes_[3])});
}

F32x4T F32x4T::blend(const F32x4T& t, const F32x4T& f) const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t maskBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t tBits = std::bit_cast<std::uint32_t>(t.lanes_[i]);
    const std::uint32_t fBits = std::bit_cast<std::uint32_t>(f.lanes_[i]);
    out[i] = std::bit_cast<float>(genericBitBlend(maskBits, tBits, fBits));
  }
  return F32x4T(out);
}

F32x4T F32x4T::floor() const {
  return F32x4T({std::floor(lanes_[0]), std::floor(lanes_[1]),
                 std::floor(lanes_[2]), std::floor(lanes_[3])});
}

F32x4T F32x4T::fract() const {
  return *this - floor();
}

F32x4T F32x4T::normalize() const {
  return max(F32x4T::splat(0.0f)).min(F32x4T::splat(1.0f));
}

F32x4T F32x4T::round() const {
  return F32x4T({std::nearbyint(lanes_[0]), std::nearbyint(lanes_[1]),
                 std::nearbyint(lanes_[2]), std::nearbyint(lanes_[3])});
}

I32x4T F32x4T::roundInt() const {
  return I32x4T({
      static_cast<std::int32_t>(std::nearbyint(lanes_[0])),
      static_cast<std::int32_t>(std::nearbyint(lanes_[1])),
      static_cast<std::int32_t>(std::nearbyint(lanes_[2])),
      static_cast<std::int32_t>(std::nearbyint(lanes_[3])),
  });
}

I32x4T F32x4T::truncInt() const {
  return I32x4T({
      static_cast<std::int32_t>(std::trunc(lanes_[0])),
      static_cast<std::int32_t>(std::trunc(lanes_[1])),
      static_cast<std::int32_t>(std::trunc(lanes_[2])),
      static_cast<std::int32_t>(std::trunc(lanes_[3])),
  });
}

I32x4T F32x4T::toI32x4Bitcast() const {
  return I32x4T({
      std::bit_cast<std::int32_t>(lanes_[0]),
      std::bit_cast<std::int32_t>(lanes_[1]),
      std::bit_cast<std::int32_t>(lanes_[2]),
      std::bit_cast<std::int32_t>(lanes_[3]),
  });
}

F32x4T F32x4T::recipFast() const {
  return F32x4T({1.0f / lanes_[0], 1.0f / lanes_[1],
                 1.0f / lanes_[2], 1.0f / lanes_[3]});
}

F32x4T F32x4T::recipSqrt() const {
  return F32x4T({1.0f / std::sqrt(lanes_[0]), 1.0f / std::sqrt(lanes_[1]),
                 1.0f / std::sqrt(lanes_[2]), 1.0f / std::sqrt(lanes_[3])});
}

F32x4T F32x4T::sqrt() const {
  return F32x4T({std::sqrt(lanes_[0]), std::sqrt(lanes_[1]),
                 std::sqrt(lanes_[2]), std::sqrt(lanes_[3])});
}

F32x4T F32x4T::operator+(const F32x4T& rhs) const {
  return F32x4T({lanes_[0] + rhs.lanes_[0], lanes_[1] + rhs.lanes_[1],
                 lanes_[2] + rhs.lanes_[2], lanes_[3] + rhs.lanes_[3]});
}

F32x4T F32x4T::operator-(const F32x4T& rhs) const {
  return F32x4T({lanes_[0] - rhs.lanes_[0], lanes_[1] - rhs.lanes_[1],
                 lanes_[2] - rhs.lanes_[2], lanes_[3] - rhs.lanes_[3]});
}

F32x4T F32x4T::operator*(const F32x4T& rhs) const {
  return F32x4T({lanes_[0] * rhs.lanes_[0], lanes_[1] * rhs.lanes_[1],
                 lanes_[2] * rhs.lanes_[2], lanes_[3] * rhs.lanes_[3]});
}

F32x4T F32x4T::operator/(const F32x4T& rhs) const {
  return F32x4T({lanes_[0] / rhs.lanes_[0], lanes_[1] / rhs.lanes_[1],
                 lanes_[2] / rhs.lanes_[2], lanes_[3] / rhs.lanes_[3]});
}

F32x4T F32x4T::operator&(const F32x4T& rhs) const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(lanes_[i]) &
                                  std::bit_cast<std::uint32_t>(rhs.lanes_[i]));
  }
  return F32x4T(out);
}

F32x4T F32x4T::operator|(const F32x4T& rhs) const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(lanes_[i]) |
                                  std::bit_cast<std::uint32_t>(rhs.lanes_[i]));
  }
  return F32x4T(out);
}

F32x4T F32x4T::operator^(const F32x4T& rhs) const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(lanes_[i]) ^
                                  std::bit_cast<std::uint32_t>(rhs.lanes_[i]));
  }
  return F32x4T(out);
}

F32x4T F32x4T::operator-() const {
  return F32x4T::splat(0.0f) - *this;
}

F32x4T F32x4T::operator~() const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(~std::bit_cast<std::uint32_t>(lanes_[i]));
  }
  return F32x4T(out);
}

F32x4T& F32x4T::operator+=(const F32x4T& rhs) {
  *this = *this + rhs;
  return *this;
}

F32x4T& F32x4T::operator*=(const F32x4T& rhs) {
  *this = *this * rhs;
  return *this;
}

bool F32x4T::operator==(const F32x4T& rhs) const {
  return lanes_ == rhs.lanes_;
}

}  // namespace tiny_skia::wide
