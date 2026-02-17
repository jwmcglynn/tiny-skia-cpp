#pragma once

#include <cmath>

namespace tiny_skia {

struct Point {
  float x = 0.0f;
  float y = 0.0f;

  [[nodiscard]] static constexpr Point fromXy(float x, float y) {
    return Point{x, y};
  }

  [[nodiscard]] float length() const {
    return std::sqrt(x * x + y * y);
  }

  void scale(float factor) {
    x *= factor;
    y *= factor;
  }

  constexpr bool operator==(const Point&) const = default;
};

[[nodiscard]] inline constexpr Point operator-(const Point& a, const Point& b) {
  return Point{a.x - b.x, a.y - b.y};
}

[[nodiscard]] inline constexpr Point operator+(const Point& a, const Point& b) {
  return Point{a.x + b.x, a.y + b.y};
}

}  // namespace tiny_skia
