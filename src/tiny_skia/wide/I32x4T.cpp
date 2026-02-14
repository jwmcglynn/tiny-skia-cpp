#include "tiny_skia/wide/I32x4T.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "tiny_skia/wide/F32x4T.h"
#include "tiny_skia/wide/Mod.h"

namespace tiny_skia::wide {

I32x4T I32x4T::blend(const I32x4T& t, const I32x4T& f) const {
  std::array<std::int32_t, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t maskBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t tBits = std::bit_cast<std::uint32_t>(t.lanes_[i]);
    const std::uint32_t fBits = std::bit_cast<std::uint32_t>(f.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(genericBitBlend(maskBits, tBits, fBits));
  }

  return I32x4T(out);
}

I32x4T I32x4T::cmpEq(const I32x4T& rhs) const {
  return I32x4T({cmpMask(lanes_[0] == rhs.lanes_[0]), cmpMask(lanes_[1] == rhs.lanes_[1]),
                 cmpMask(lanes_[2] == rhs.lanes_[2]), cmpMask(lanes_[3] == rhs.lanes_[3])});
}

I32x4T I32x4T::cmpGt(const I32x4T& rhs) const {
  return I32x4T({cmpMask(lanes_[0] > rhs.lanes_[0]), cmpMask(lanes_[1] > rhs.lanes_[1]),
                 cmpMask(lanes_[2] > rhs.lanes_[2]), cmpMask(lanes_[3] > rhs.lanes_[3])});
}

I32x4T I32x4T::cmpLt(const I32x4T& rhs) const {
  return I32x4T({cmpMask(lanes_[0] < rhs.lanes_[0]), cmpMask(lanes_[1] < rhs.lanes_[1]),
                 cmpMask(lanes_[2] < rhs.lanes_[2]), cmpMask(lanes_[3] < rhs.lanes_[3])});
}

F32x4T I32x4T::toF32x4() const {
  return F32x4T({static_cast<float>(lanes_[0]), static_cast<float>(lanes_[1]),
                 static_cast<float>(lanes_[2]), static_cast<float>(lanes_[3])});
}

F32x4T I32x4T::toF32x4Bitcast() const {
  return F32x4T({std::bit_cast<float>(lanes_[0]), std::bit_cast<float>(lanes_[1]),
                 std::bit_cast<float>(lanes_[2]), std::bit_cast<float>(lanes_[3])});
}

I32x4T I32x4T::operator+(const I32x4T& rhs) const {
  std::array<std::int32_t, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(lhsBits + rhsBits);
  }

  return I32x4T(out);
}

I32x4T I32x4T::operator*(const I32x4T& rhs) const {
  std::array<std::int32_t, 4> out{};
  for (std::size_t i = 0; i < out.size(); ++i) {
    const std::uint32_t lhsBits = std::bit_cast<std::uint32_t>(lanes_[i]);
    const std::uint32_t rhsBits = std::bit_cast<std::uint32_t>(rhs.lanes_[i]);
    out[i] = std::bit_cast<std::int32_t>(lhsBits * rhsBits);
  }

  return I32x4T(out);
}

I32x4T I32x4T::operator&(const I32x4T& rhs) const {
  return I32x4T({lanes_[0] & rhs.lanes_[0], lanes_[1] & rhs.lanes_[1], lanes_[2] & rhs.lanes_[2],
                 lanes_[3] & rhs.lanes_[3]});
}

I32x4T I32x4T::operator|(const I32x4T& rhs) const {
  return I32x4T({lanes_[0] | rhs.lanes_[0], lanes_[1] | rhs.lanes_[1], lanes_[2] | rhs.lanes_[2],
                 lanes_[3] | rhs.lanes_[3]});
}

I32x4T I32x4T::operator^(const I32x4T& rhs) const {
  return I32x4T({lanes_[0] ^ rhs.lanes_[0], lanes_[1] ^ rhs.lanes_[1], lanes_[2] ^ rhs.lanes_[2],
                 lanes_[3] ^ rhs.lanes_[3]});
}

}  // namespace tiny_skia::wide
