#include "tiny_skia/wide/I32x8T.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/F32x8T.h"
#include "tiny_skia/wide/Mod.h"
#include "tiny_skia/wide/U32x8T.h"

namespace tiny_skia::wide {

I32x8T I32x8T::blend(const I32x8T& t, const I32x8T& f) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t maskBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t tBits = std::bit_cast<std::uint32_t>(t.lanes_[i]);
    const std::uint32_t fBits = std::bit_cast<std::uint32_t>(f.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(genericBitBlend(maskBits, tBits, fBits));
  }

  return I32x8T(out);
}

I32x8T I32x8T::cmpEq(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] == rhs.lanes_[i]);
  }

  return I32x8T(out);
}

I32x8T I32x8T::cmpGt(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] > rhs.lanes_[i]);
  }

  return I32x8T(out);
}

I32x8T I32x8T::cmpLt(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = cmpMask(lanes_[i] < rhs.lanes_[i]);
  }

  return I32x8T(out);
}

F32x8T I32x8T::toF32x8() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<float>(lanes_[i]);
  }

  return F32x8T(out);
}

U32x8T I32x8T::toU32x8Bitcast() const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<std::uint32_t>(lanes_[i]);
  }

  return U32x8T(out);
}

F32x8T I32x8T::toF32x8Bitcast() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(lanes_[i]);
  }

  return F32x8T(out);
}

I32x8T I32x8T::operator+(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(lhsBits + rhsBits);
  }

  return I32x8T(out);
}

I32x8T I32x8T::operator*(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(lhsBits * rhsBits);
  }

  return I32x8T(out);
}

I32x8T I32x8T::operator&(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] & rhs.lanes_[i];
  }

  return I32x8T(out);
}

I32x8T I32x8T::operator|(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] | rhs.lanes_[i];
  }

  return I32x8T(out);
}

I32x8T I32x8T::operator^(const I32x8T& rhs) const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] ^ rhs.lanes_[i];
  }

  return I32x8T(out);
}

}  // namespace tiny_skia::wide
