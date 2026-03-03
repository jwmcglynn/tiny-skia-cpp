#pragma once

#include <array>
#include <cstdint>

#include "tiny_skia/wide/backend/ScalarU16x16T.h"

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace tiny_skia::wide::backend::aarch64_neon {

#if defined(__aarch64__) && defined(__ARM_NEON)

struct U16x16Neon {
  uint16x8_t lo;
  uint16x8_t hi;
};

[[maybe_unused]] [[nodiscard]] inline U16x16Neon splitU16x16(const U16x16T& value) {
  const auto pair = vld1q_u16_x2(value.lanes().data());
  return U16x16Neon{pair.val[0], pair.val[1]};
}

[[maybe_unused]] [[nodiscard]] inline U16x16T joinU16x16(const U16x16Neon& value) {
  std::array<std::uint16_t, 16> out{};
  const uint16x8x2_t pair = {{value.lo, value.hi}};
  vst1q_u16_x2(out.data(), pair);
  return U16x16T(out);
}

[[nodiscard]] inline U16x16Neon div255Neon(const U16x16Neon& value) {
  const auto bias = vdupq_n_u16(255);
  return U16x16Neon{vshrq_n_u16(vaddq_u16(value.lo, bias), 8),
                    vshrq_n_u16(vaddq_u16(value.hi, bias), 8)};
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Min(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vminq_u16(a.lo, b.lo), vminq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Max(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vmaxq_u16(a.lo, b.lo), vmaxq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16CmpLe(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vcleq_u16(a.lo, b.lo), vcleq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Blend(const U16x16T& mask,
                                                           const U16x16T& t,
                                                           const U16x16T& e) {
  const auto m = splitU16x16(mask);
  const auto onTrue = splitU16x16(t);
  const auto onFalse = splitU16x16(e);
  return joinU16x16(U16x16Neon{vbslq_u16(m.lo, onTrue.lo, onFalse.lo),
                               vbslq_u16(m.hi, onTrue.hi, onFalse.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Add(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vaddq_u16(a.lo, b.lo), vaddq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Sub(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vsubq_u16(a.lo, b.lo), vsubq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Mul(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vmulq_u16(a.lo, b.lo), vmulq_u16(a.hi, b.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16And(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vandq_u16(a.lo, b.lo), vandq_u16(a.hi, b.hi)});
}


[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Div255(const U16x16T& value) {
  return joinU16x16(div255Neon(splitU16x16(value)));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16MulDiv255(const U16x16T& lhs,
                                                               const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(div255Neon(U16x16Neon{vmulq_u16(a.lo, b.lo), vmulq_u16(a.hi, b.hi)}));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16MulAddDiv255(const U16x16T& lhs0,
                                                                  const U16x16T& rhs0,
                                                                  const U16x16T& lhs1,
                                                                  const U16x16T& rhs1) {
  const auto a0 = splitU16x16(lhs0);
  const auto b0 = splitU16x16(rhs0);
  const auto a1 = splitU16x16(lhs1);
  const auto b1 = splitU16x16(rhs1);
  return joinU16x16(div255Neon(U16x16Neon{
      vaddq_u16(vmulq_u16(a0.lo, b0.lo), vmulq_u16(a1.lo, b1.lo)),
      vaddq_u16(vmulq_u16(a0.hi, b0.hi), vmulq_u16(a1.hi, b1.hi)),
  }));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16SourceOver(const U16x16T& source,
                                                                const U16x16T& dest,
                                                                const U16x16T& sourceAlpha) {
  const auto s = splitU16x16(source);
  const auto d = splitU16x16(dest);
  const auto sa = splitU16x16(sourceAlpha);
  const auto max255 = vdupq_n_u16(255);
  const U16x16Neon invSa{vsubq_u16(max255, sa.lo), vsubq_u16(max255, sa.hi)};
  const U16x16Neon dstTerm = div255Neon(
      U16x16Neon{vmulq_u16(d.lo, invSa.lo), vmulq_u16(d.hi, invSa.hi)});
  return joinU16x16(U16x16Neon{vaddq_u16(s.lo, dstTerm.lo), vaddq_u16(s.hi, dstTerm.hi)});
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Or(const U16x16T& lhs, const U16x16T& rhs) {
  const auto a = splitU16x16(lhs);
  const auto b = splitU16x16(rhs);
  return joinU16x16(U16x16Neon{vorrq_u16(a.lo, b.lo), vorrq_u16(a.hi, b.hi)});
}

#else

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Min(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Min(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Max(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Max(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16CmpLe(const U16x16T& lhs,
                                                           const U16x16T& rhs) {
  return scalar::u16x16CmpLe(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Blend(const U16x16T& mask,
                                                           const U16x16T& t,
                                                           const U16x16T& e) {
  return scalar::u16x16Blend(mask, t, e);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Add(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Add(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Sub(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Sub(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Mul(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Mul(lhs, rhs);
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16And(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16And(lhs, rhs);
}


[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Div255(const U16x16T& value) {
  return scalar::u16x16Shr(scalar::u16x16Add(value, U16x16T::splat(255)),
                           U16x16T::splat(8));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16MulDiv255(const U16x16T& lhs,
                                                               const U16x16T& rhs) {
  return u16x16Div255(scalar::u16x16Mul(lhs, rhs));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16MulAddDiv255(const U16x16T& lhs0,
                                                                  const U16x16T& rhs0,
                                                                  const U16x16T& lhs1,
                                                                  const U16x16T& rhs1) {
  return u16x16Div255(scalar::u16x16Add(scalar::u16x16Mul(lhs0, rhs0),
                                        scalar::u16x16Mul(lhs1, rhs1)));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16SourceOver(const U16x16T& source,
                                                                const U16x16T& dest,
                                                                const U16x16T& sourceAlpha) {
  return scalar::u16x16Add(
      source,
      u16x16Div255(
          scalar::u16x16Mul(dest, scalar::u16x16Sub(U16x16T::splat(255), sourceAlpha))));
}

[[maybe_unused]] [[nodiscard]] inline U16x16T u16x16Or(const U16x16T& lhs, const U16x16T& rhs) {
  return scalar::u16x16Or(lhs, rhs);
}

#endif

}  // namespace tiny_skia::wide::backend::aarch64_neon
