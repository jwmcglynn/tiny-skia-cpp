#include "tiny_skia/wide/U16x16T.h"

#include "tiny_skia/wide/backend/Aarch64NeonU16x16T.h"
#include "tiny_skia/wide/backend/ScalarU16x16T.h"

namespace tiny_skia::wide {

namespace {

[[nodiscard]] constexpr bool useAarch64NeonU16x16() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return true;
#else
  return false;
#endif
}

}  // namespace

U16x16T U16x16T::min(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Min(*this, rhs);
  }

  return backend::scalar::u16x16Min(*this, rhs);
}

U16x16T U16x16T::max(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Max(*this, rhs);
  }

  return backend::scalar::u16x16Max(*this, rhs);
}

U16x16T U16x16T::cmpLe(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16CmpLe(*this, rhs);
  }

  return backend::scalar::u16x16CmpLe(*this, rhs);
}

U16x16T U16x16T::blend(const U16x16T& t, const U16x16T& e) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Blend(*this, t, e);
  }

  return backend::scalar::u16x16Blend(*this, t, e);
}

U16x16T U16x16T::operator+(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Add(*this, rhs);
  }

  return backend::scalar::u16x16Add(*this, rhs);
}

U16x16T U16x16T::operator-(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Sub(*this, rhs);
  }

  return backend::scalar::u16x16Sub(*this, rhs);
}

U16x16T U16x16T::operator*(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Mul(*this, rhs);
  }

  return backend::scalar::u16x16Mul(*this, rhs);
}

U16x16T U16x16T::operator/(const U16x16T& rhs) const {
  return backend::scalar::u16x16Div(*this, rhs);
}

U16x16T U16x16T::operator&(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16And(*this, rhs);
  }

  return backend::scalar::u16x16And(*this, rhs);
}

U16x16T U16x16T::operator|(const U16x16T& rhs) const {
  if constexpr (useAarch64NeonU16x16()) {
    return backend::aarch64_neon::u16x16Or(*this, rhs);
  }

  return backend::scalar::u16x16Or(*this, rhs);
}

U16x16T U16x16T::operator~() const {
  return backend::scalar::u16x16Not(*this);
}

U16x16T U16x16T::operator>>(const U16x16T& rhs) const {
  return backend::scalar::u16x16Shr(*this, rhs);
}

}  // namespace tiny_skia::wide
