#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/Mod.h"

namespace tiny_skia::wide {

class U16x16T {
 public:
  U16x16T() = default;
  explicit constexpr U16x16T(std::array<std::uint16_t, 16> lanes)
      : lanes_(lanes) {}

  [[nodiscard]] static constexpr U16x16T splat(std::uint16_t n) {
    return U16x16T({n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n});
  }

  [[nodiscard]] constexpr const std::array<std::uint16_t, 16>& lanes() const {
    return lanes_;
  }

  [[nodiscard]] constexpr std::array<std::uint16_t, 16>& lanes() {
    return lanes_;
  }

  [[nodiscard]] U16x16T min(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T max(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T cmpLe(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T blend(const U16x16T& t, const U16x16T& e) const;

  [[nodiscard]] U16x16T operator+(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator-(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator*(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator/(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator&(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator|(const U16x16T& rhs) const;
  [[nodiscard]] U16x16T operator~() const;
  [[nodiscard]] U16x16T operator>>(const U16x16T& rhs) const;

 private:
  std::array<std::uint16_t, 16> lanes_{};
};

}  // namespace tiny_skia::wide
