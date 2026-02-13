#include "tiny_skia/path64/Point64.h"

namespace tiny_skia {

Point Point64::toPoint() const {
  return Point{static_cast<float>(x), static_cast<float>(y)};
}

double Point64::axisCoord(SearchAxis axis) const {
  return axis == SearchAxis::X ? x : y;
}

}  // namespace tiny_skia
