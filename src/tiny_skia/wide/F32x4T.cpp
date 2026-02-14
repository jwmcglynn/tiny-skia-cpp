#include "tiny_skia/wide/F32x4T.h"

#include <bit>
#include <cmath>
#include <cstdint>

#include "tiny_skia/wide/Mod.h"

namespace tiny_skia::wide {

float F32x4T::cmpMask(bool predicate) {
  return predicate ? kTrueMask : 0.0f;
}

F32x4T F32x4T::abs() const {
  return F32x4T({std::fabs(lanes_[0]), std::fabs(lanes_[1]), std::fabs(lanes_[2]),
                 std::fabs(lanes_[3])});
}

F32x4T F32x4T::max(const F32x4T& rhs) const {
  return F32x4T({fasterMax(lanes_[0], rhs.lanes_[0]), fasterMax(lanes_[1], rhs.lanes_[1]),
                 fasterMax(lanes_[2], rhs.lanes_[2]), fasterMax(lanes_[3], rhs.lanes_[3])});
}

F32x4T F32x4T::min(const F32x4T& rhs) const {
  return F32x4T({fasterMin(lanes_[0], rhs.lanes_[0]), fasterMin(lanes_[1], rhs.lanes_[1]),
                 fasterMin(lanes_[2], rhs.lanes_[2]), fasterMin(lanes_[3], rhs.lanes_[3])});
}

F32x4T F32x4T::cmpEq(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] == rhs.lanes_[0]), cmpMask(lanes_[1] == rhs.lanes_[1]),
                 cmpMask(lanes_[2] == rhs.lanes_[2]), cmpMask(lanes_[3] == rhs.lanes_[3])});
}

F32x4T F32x4T::cmpNe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] != rhs.lanes_[0]), cmpMask(lanes_[1] != rhs.lanes_[1]),
                 cmpMask(lanes_[2] != rhs.lanes_[2]), cmpMask(lanes_[3] != rhs.lanes_[3])});
}

F32x4T F32x4T::cmpGe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] >= rhs.lanes_[0]), cmpMask(lanes_[1] >= rhs.lanes_[1]),
                 cmpMask(lanes_[2] >= rhs.lanes_[2]), cmpMask(lanes_[3] >= rhs.lanes_[3])});
}

F32x4T F32x4T::cmpGt(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] > rhs.lanes_[0]), cmpMask(lanes_[1] > rhs.lanes_[1]),
                 cmpMask(lanes_[2] > rhs.lanes_[2]), cmpMask(lanes_[3] > rhs.lanes_[3])});
}

F32x4T F32x4T::cmpLe(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] <= rhs.lanes_[0]), cmpMask(lanes_[1] <= rhs.lanes_[1]),
                 cmpMask(lanes_[2] <= rhs.lanes_[2]), cmpMask(lanes_[3] <= rhs.lanes_[3])});
}

F32x4T F32x4T::cmpLt(const F32x4T& rhs) const {
  return F32x4T({cmpMask(lanes_[0] < rhs.lanes_[0]), cmpMask(lanes_[1] < rhs.lanes_[1]),
                 cmpMask(lanes_[2] < rhs.lanes_[2]), cmpMask(lanes_[3] < rhs.lanes_[3])});
}

F32x4T F32x4T::blend(const F32x4T& t, const F32x4T& f) const {
  std::array<float, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t maskBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t tBits = std::bit_cast<std::uint32_t>(t.lanes_[i]);
    const std::uint32_t fBits = std::bit_cast<std::uint32_t>(f.lanes_[i]);
    out[i] = std::bit_cast<float>(genericBitBlend(maskBits, tBits, fBits));
  }

  return F32x4T(out);
}

}  // namespace tiny_skia::wide
