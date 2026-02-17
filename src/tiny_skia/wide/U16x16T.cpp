#include "tiny_skia/wide/U16x16T.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace tiny_skia::wide {

U16x16T U16x16T::min(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::min(lanes_[i], rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::max(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = std::max(lanes_[i], rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::cmpLe(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = lanes_[i] <= rhs.lanes_[i] ? UINT16_MAX : 0;
  }

  return U16x16T(out);
}

U16x16T U16x16T::blend(const U16x16T& t, const U16x16T& e) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = genericBitBlend(lanes_[i], t.lanes_[i], e.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator+(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] + rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator-(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] - rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator*(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] * rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator/(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] / rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator&(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] & rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator|(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] | rhs.lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator~() const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(~lanes_[i]);
  }

  return U16x16T(out);
}

U16x16T U16x16T::operator>>(const U16x16T& rhs) const {
  std::array<std::uint16_t, 16> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    out[i] = static_cast<std::uint16_t>(lanes_[i] >> rhs.lanes_[i]);
  }

  return U16x16T(out);
}

}  // namespace tiny_skia::wide
