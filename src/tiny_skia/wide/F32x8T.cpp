#include "tiny_skia/wide/F32x8T.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/I32x8T.h"
#include "tiny_skia/wide/Mod.h"
#include "tiny_skia/wide/U32x8T.h"

namespace tiny_skia::wide {

float F32x8T::cmpMask(bool predicate) {
  return predicate ? kTrueMask : 0.0f;
}

F32x8T F32x8T::floor() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::floor(lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::fract() const {
  return *this - floor();
}

F32x8T F32x8T::normalize() const {
  return max(F32x8T::splat(0.0f)).min(F32x8T::splat(1.0f));
}

I32x8T F32x8T::toI32x8Bitcast() const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<std::int32_t>(lanes_[i]);
  }

  return I32x8T(out);
}

U32x8T F32x8T::toU32x8Bitcast() const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<std::uint32_t>(lanes_[i]);
  }

  return U32x8T(out);
}

F32x8T F32x8T::cmpEq(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] == rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::cmpNe(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] != rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::cmpGe(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] >= rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::cmpGt(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] > rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::cmpLe(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] <= rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::cmpLt(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] < rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::blend(const F32x8T& t, const F32x8T& f) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t maskBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t tBits = std::bit_cast<std::uint32_t>(t.lanes_[i]);
    const std::uint32_t fBits = std::bit_cast<std::uint32_t>(f.lanes_[i]);
    out[i] = std::bit_cast<float>(genericBitBlend(maskBits, tBits, fBits));
  }

  return F32x8T(out);
}

F32x8T F32x8T::abs() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::fabs(lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::max(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = fasterMax(lanes_[i], rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::min(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = fasterMin(lanes_[i], rhs.lanes_[i]);
  }

  return F32x8T(out);
}

F32x8T F32x8T::isFinite() const {
  const U32x8T shiftedExpMask = U32x8T::splat(0xFF000000u);
  const U32x8T u = toU32x8Bitcast();
  const U32x8T shiftU = u.shl<1>();
  const U32x8T out = ~(shiftU & shiftedExpMask).cmpEq(shiftedExpMask);
  return out.toF32x8Bitcast();
}

F32x8T F32x8T::round() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::nearbyint(lanes_[i]);
  }

  return F32x8T(out);
}

I32x8T F32x8T::roundInt() const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::int32_t>(std::nearbyint(lanes_[i]));
  }

  return I32x8T(out);
}

I32x8T F32x8T::truncInt() const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::int32_t>(std::trunc(lanes_[i]));
  }

  return I32x8T(out);
}

F32x8T F32x8T::operator+(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] + rhs.lanes_[i];
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator-(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] - rhs.lanes_[i];
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator*(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] * rhs.lanes_[i];
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator/(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] / rhs.lanes_[i];
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator&(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<float>(lhsBits & rhsBits);
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator|(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<float>(lhsBits | rhsBits);
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator^(const F32x8T& rhs) const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<float>(lhsBits ^ rhsBits);
  }

  return F32x8T(out);
}

F32x8T F32x8T::operator-() const {
  return F32x8T::splat(0.0f) - *this;
}

F32x8T F32x8T::operator~() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(lanes_[i]);
    out[i] = std::bit_cast<float>(~bits);
  }

  return F32x8T(out);
}

bool F32x8T::operator==(const F32x8T& rhs) const {
  return lanes_ == rhs.lanes_;
}

}  // namespace tiny_skia::wide
