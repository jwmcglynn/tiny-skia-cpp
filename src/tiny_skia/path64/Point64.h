#pragma once

#include "tiny_skia/Edge.h"

namespace tiny_skia {

enum class SearchAxis {
  X,
  Y,
};

struct Point64 {
  double x = 0.0;
  double y = 0.0;

  static constexpr Point64 fromXy(double x, double y) {
    return Point64{x, y};
  }

  static Point64 fromPoint(const Point& point) {
    return Point64{static_cast<double>(point.x), static_cast<double>(point.y)};
  }

  static constexpr Point64 zero() {
    return Point64{};
  }

  Point toPoint() const;

  [[nodiscard]] double axisCoord(SearchAxis axis) const;
};

}  // namespace tiny_skia
