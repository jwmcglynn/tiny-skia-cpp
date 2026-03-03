#include "tiny_skia/wide/U32x8T.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/F32x8T.h"
#include "tiny_skia/wide/I32x8T.h"

namespace tiny_skia::wide {

I32x8T U32x8T::toI32x8Bitcast() const {
  std::array<std::int32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<std::int32_t>(lanes_[i]);
  }

  return I32x8T(out);
}

F32x8T U32x8T::toF32x8Bitcast() const {
  std::array<float, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::bit_cast<float>(lanes_[i]);
  }

  return F32x8T(out);
}

U32x8T U32x8T::cmpEq(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] == rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::cmpNe(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] != rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::cmpLt(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] < rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::cmpLe(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] <= rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::cmpGt(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] > rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::cmpGe(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] >= rhs.lanes_[i] ? UINT32_MAX : 0;
  }

  return U32x8T(out);
}

U32x8T U32x8T::operator~() const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = ~lanes_[i];
  }

  return U32x8T(out);
}

U32x8T U32x8T::operator+(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] + rhs.lanes_[i];
  }

  return U32x8T(out);
}

U32x8T U32x8T::operator&(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] & rhs.lanes_[i];
  }

  return U32x8T(out);
}

U32x8T U32x8T::operator|(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] | rhs.lanes_[i];
  }

  return U32x8T(out);
}

U32x8T U32x8T::operator^(const U32x8T& rhs) const {
  std::array<std::uint32_t, 8> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] ^ rhs.lanes_[i];
  }

  return U32x8T(out);
}

}  // namespace tiny_skia::wide
