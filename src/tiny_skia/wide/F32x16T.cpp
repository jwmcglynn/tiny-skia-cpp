#include "tiny_skia/wide/F32x16T.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/F32x8T.h"
#include "tiny_skia/wide/U16x16T.h"

namespace tiny_skia::wide {

F32x16T F32x16T::abs() const {
  // Yes, Skia does it in the same way.
  const auto absF = [](float x) -> float {
    return std::bit_cast<float>(std::bit_cast<std::int32_t>(x) & 0x7fffffff);
  };

  const auto n0 = lo_.lanes();
  const auto n1 = hi_.lanes();
  return F32x16T(
      F32x8T({absF(n0[0]), absF(n0[1]), absF(n0[2]), absF(n0[3]),
              absF(n0[4]), absF(n0[5]), absF(n0[6]), absF(n0[7])}),
      F32x8T({absF(n1[0]), absF(n1[1]), absF(n1[2]), absF(n1[3]),
              absF(n1[4]), absF(n1[5]), absF(n1[6]), absF(n1[7])}));
}

F32x16T F32x16T::cmpGt(const F32x16T& rhs) const {
  return F32x16T(lo_.cmpGt(rhs.lo_), hi_.cmpGt(rhs.hi_));
}

F32x16T F32x16T::blend(const F32x16T& t, const F32x16T& f) const {
  return F32x16T(lo_.blend(t.lo_, f.lo_), hi_.blend(t.hi_, f.hi_));
}

F32x16T F32x16T::normalize() const {
  return F32x16T(lo_.normalize(), hi_.normalize());
}

F32x16T F32x16T::floor() const {
  // Yes, Skia does it in the same way.
  const F32x16T roundtrip = round();
  return roundtrip -
         roundtrip.cmpGt(*this).blend(F32x16T::splat(1.0f),
                                      F32x16T::splat(0.0f));
}

F32x16T F32x16T::sqrt() const {
  const auto n0 = lo_.lanes();
  const auto n1 = hi_.lanes();
  return F32x16T(
      F32x8T({std::sqrt(n0[0]), std::sqrt(n0[1]), std::sqrt(n0[2]),
              std::sqrt(n0[3]), std::sqrt(n0[4]), std::sqrt(n0[5]),
              std::sqrt(n0[6]), std::sqrt(n0[7])}),
      F32x8T({std::sqrt(n1[0]), std::sqrt(n1[1]), std::sqrt(n1[2]),
              std::sqrt(n1[3]), std::sqrt(n1[4]), std::sqrt(n1[5]),
              std::sqrt(n1[6]), std::sqrt(n1[7])}));
}

F32x16T F32x16T::round() const {
  return F32x16T(lo_.round(), hi_.round());
}

// This method is too heavy and shouldn't be inlined.
void F32x16T::saveToU16x16(U16x16T& dst) const {
  // Do not use roundInt, because it involves rounding,
  // and Skia casts without it.

  const auto n0 = lo_.lanes();
  const auto n1 = hi_.lanes();

  auto& lanes = dst.lanes();
  lanes[0] = static_cast<std::uint16_t>(n0[0]);
  lanes[1] = static_cast<std::uint16_t>(n0[1]);
  lanes[2] = static_cast<std::uint16_t>(n0[2]);
  lanes[3] = static_cast<std::uint16_t>(n0[3]);
  lanes[4] = static_cast<std::uint16_t>(n0[4]);
  lanes[5] = static_cast<std::uint16_t>(n0[5]);
  lanes[6] = static_cast<std::uint16_t>(n0[6]);
  lanes[7] = static_cast<std::uint16_t>(n0[7]);

  lanes[8] = static_cast<std::uint16_t>(n1[0]);
  lanes[9] = static_cast<std::uint16_t>(n1[1]);
  lanes[10] = static_cast<std::uint16_t>(n1[2]);
  lanes[11] = static_cast<std::uint16_t>(n1[3]);
  lanes[12] = static_cast<std::uint16_t>(n1[4]);
  lanes[13] = static_cast<std::uint16_t>(n1[5]);
  lanes[14] = static_cast<std::uint16_t>(n1[6]);
  lanes[15] = static_cast<std::uint16_t>(n1[7]);
}

F32x16T F32x16T::operator+(const F32x16T& rhs) const {
  return F32x16T(lo_ + rhs.lo_, hi_ + rhs.hi_);
}

F32x16T F32x16T::operator-(const F32x16T& rhs) const {
  return F32x16T(lo_ - rhs.lo_, hi_ - rhs.hi_);
}

F32x16T F32x16T::operator*(const F32x16T& rhs) const {
  return F32x16T(lo_ * rhs.lo_, hi_ * rhs.hi_);
}

}  // namespace tiny_skia::wide
