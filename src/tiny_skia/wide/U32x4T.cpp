#include "tiny_skia/wide/U32x4T.h"

#include "tiny_skia/wide/backend/Aarch64NeonU32x4T.h"
#include "tiny_skia/wide/backend/ScalarU32x4T.h"

namespace tiny_skia::wide {

namespace {

[[nodiscard]] constexpr bool useAarch64NeonU32x4() {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE) && defined(__aarch64__) && defined(__ARM_NEON)
  return true;
#else
  return false;
#endif
}

}  // namespace

U32x4T U32x4T::cmpEq(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpEq(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpEq(lanes_, rhs.lanes_));
}

U32x4T U32x4T::cmpNe(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpNe(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpNe(lanes_, rhs.lanes_));
}

U32x4T U32x4T::cmpLt(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpLt(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpLt(lanes_, rhs.lanes_));
}

U32x4T U32x4T::cmpLe(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpLe(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpLe(lanes_, rhs.lanes_));
}

U32x4T U32x4T::cmpGt(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpGt(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpGt(lanes_, rhs.lanes_));
}

U32x4T U32x4T::cmpGe(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4CmpGe(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4CmpGe(lanes_, rhs.lanes_));
}

U32x4T U32x4T::operator~() const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4Not(lanes_));
  }

  return U32x4T(backend::scalar::u32x4Not(lanes_));
}

U32x4T U32x4T::operator+(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4Add(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4Add(lanes_, rhs.lanes_));
}

U32x4T U32x4T::operator&(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4And(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4And(lanes_, rhs.lanes_));
}

U32x4T U32x4T::operator|(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4Or(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4Or(lanes_, rhs.lanes_));
}

U32x4T U32x4T::operator^(const U32x4T& rhs) const {
  if constexpr (useAarch64NeonU32x4()) {
    return U32x4T(backend::aarch64_neon::u32x4Xor(lanes_, rhs.lanes_));
  }

  return U32x4T(backend::scalar::u32x4Xor(lanes_, rhs.lanes_));
}

}  // namespace tiny_skia::wide
