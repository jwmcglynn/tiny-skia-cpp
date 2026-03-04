#include "tiny_skia/wide/F32x16T.h"

#include "tiny_skia/wide/backend/ScalarF32x16T.h"

namespace tiny_skia::wide {

F32x16T F32x16T::abs() const { return F32x16T(lo_.abs(), hi_.abs()); }

F32x16T F32x16T::cmpGt(const F32x16T& rhs) const {
  return F32x16T(lo_.cmpGt(rhs.lo_), hi_.cmpGt(rhs.hi_));
}

F32x16T F32x16T::blend(const F32x16T& t, const F32x16T& f) const {
  return F32x16T(lo_.blend(t.lo_, f.lo_), hi_.blend(t.hi_, f.hi_));
}

F32x16T F32x16T::normalize() const { return F32x16T(lo_.normalize(), hi_.normalize()); }

F32x16T F32x16T::floor() const {
  // Skia computes floor via round and mask blend to avoid libm edge semantics.
  const F32x16T roundtrip = round();
  return roundtrip - roundtrip.cmpGt(*this).blend(F32x16T::splat(1.0f), F32x16T::splat(0.0f));
}

F32x16T F32x16T::sqrt() const { return F32x16T(lo_.sqrt(), hi_.sqrt()); }

F32x16T F32x16T::round() const { return F32x16T(lo_.round(), hi_.round()); }

// This method is too heavy and shouldn't be inlined.
void F32x16T::saveToU16x16(U16x16T& dst) const {
  // Do not use roundInt, because it involves rounding,
  // and Skia casts without it.
  backend::scalar::f32x16SaveToU16x16(*this, dst);
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
