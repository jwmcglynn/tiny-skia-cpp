#include "tiny_skia/wide/U32x4T.h"

#include <cstdint>

namespace tiny_skia::wide {

U32x4T U32x4T::cmpEq(const U32x4T& rhs) const {
  return U32x4T({lanes_[0] == rhs.lanes_[0] ? UINT32_MAX : 0,
                 lanes_[1] == rhs.lanes_[1] ? UINT32_MAX : 0,
                 lanes_[2] == rhs.lanes_[2] ? UINT32_MAX : 0,
                 lanes_[3] == rhs.lanes_[3] ? UINT32_MAX : 0});
}

U32x4T U32x4T::operator~() const {
  return U32x4T({~lanes_[0], ~lanes_[1], ~lanes_[2], ~lanes_[3]});
}

U32x4T U32x4T::operator+(const U32x4T& rhs) const {
  return U32x4T({lanes_[0] + rhs.lanes_[0], lanes_[1] + rhs.lanes_[1],
                 lanes_[2] + rhs.lanes_[2], lanes_[3] + rhs.lanes_[3]});
}

U32x4T U32x4T::operator&(const U32x4T& rhs) const {
  return U32x4T({lanes_[0] & rhs.lanes_[0], lanes_[1] & rhs.lanes_[1],
                 lanes_[2] & rhs.lanes_[2], lanes_[3] & rhs.lanes_[3]});
}

U32x4T U32x4T::operator|(const U32x4T& rhs) const {
  return U32x4T({lanes_[0] | rhs.lanes_[0], lanes_[1] | rhs.lanes_[1],
                 lanes_[2] | rhs.lanes_[2], lanes_[3] | rhs.lanes_[3]});
}

}  // namespace tiny_skia::wide
