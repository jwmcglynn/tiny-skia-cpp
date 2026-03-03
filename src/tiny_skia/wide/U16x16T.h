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

#include "tiny_skia/wide/backend/Aarch64NeonU16x16T.h"
#include "tiny_skia/wide/backend/ScalarU16x16T.h"
#include "tiny_skia/wide/backend/X86Avx2FmaU16x16T.h"

namespace tiny_skia::wide {

namespace {

[[nodiscard]] constexpr bool useX86Avx2FmaU16x16() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__AVX2__) && defined(__FMA__) && \
    (defined(__x86_64__) || defined(__i386__))
  return true;
#else
  return false;
#endif
}

[[nodiscard]] constexpr bool useAarch64NeonU16x16() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return true;
#else
  return false;
#endif
}

}  // namespace

inline U16x16T U16x16T::min(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Min(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Min(*this, rhs);
  }

  return backend::scalar::u16x16Min(*this, rhs);
}

inline U16x16T U16x16T::max(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Max(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Max(*this, rhs);
  }

  return backend::scalar::u16x16Max(*this, rhs);
}

inline U16x16T U16x16T::cmpLe(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16CmpLe(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16CmpLe(*this, rhs);
  }

  return backend::scalar::u16x16CmpLe(*this, rhs);
}

inline U16x16T U16x16T::blend(const U16x16T& t, const U16x16T& e) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Blend(*this, t, e);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Blend(*this, t, e);
  }

  return backend::scalar::u16x16Blend(*this, t, e);
}

inline U16x16T U16x16T::operator+(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Add(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Add(*this, rhs);
  }

  return backend::scalar::u16x16Add(*this, rhs);
}

inline U16x16T U16x16T::operator-(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Sub(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Sub(*this, rhs);
  }

  return backend::scalar::u16x16Sub(*this, rhs);
}

inline U16x16T U16x16T::operator*(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Mul(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Mul(*this, rhs);
  }

  return backend::scalar::u16x16Mul(*this, rhs);
}

inline U16x16T U16x16T::operator/(const U16x16T& rhs) const {
  return backend::scalar::u16x16Div(*this, rhs);
}

inline U16x16T U16x16T::operator&(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16And(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16And(*this, rhs);
  }

  return backend::scalar::u16x16And(*this, rhs);
}

inline U16x16T U16x16T::operator|(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Or(*this, rhs);
  }
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Or(*this, rhs);
  }

  return backend::scalar::u16x16Or(*this, rhs);
}

inline U16x16T U16x16T::operator~() const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Not(*this);
  }

  return backend::scalar::u16x16Not(*this);
}

inline U16x16T U16x16T::operator>>(const U16x16T& rhs) const {
  if constexpr (useX86Avx2FmaU16x16()) {
    return backend::x86_avx2_fma::u16x16Shr(*this, rhs);
  }

  return backend::scalar::u16x16Shr(*this, rhs);
}

}  // namespace tiny_skia::wide
