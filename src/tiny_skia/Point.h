#pragma once

#include <cmath>

namespace tiny_skia {

/// Scalar helper constant.
constexpr float kScalarRoot2Over2 = 0.707106781f;

struct Point {
  float x = 0.0f;
  float y = 0.0f;

  [[nodiscard]] static constexpr Point zero() { return Point{0.0f, 0.0f}; }

  [[nodiscard]] static constexpr Point fromXy(float px, float py) { return Point{px, py}; }

  [[nodiscard]] bool isZero() const { return x == 0.0f && y == 0.0f; }

  [[nodiscard]] bool isFinite() const { return std::isfinite(x) && std::isfinite(y); }

  [[nodiscard]] float length() const {
    // Use f32 normally, fall back to f64 on overflow.
    float mag2 = x * x + y * y;
    if (std::isfinite(mag2)) {
      return std::sqrt(mag2);
    }
    double xx = x, yy = y;
    return static_cast<float>(std::sqrt(xx * xx + yy * yy));
  }

  [[nodiscard]] float lengthSqd() const { return x * x + y * y; }

  [[nodiscard]] float distance(const Point& other) const {
    // Subtract in f32, then call length().
    return Point{x - other.x, y - other.y}.length();
  }

  [[nodiscard]] float distanceToSqd(const Point& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return dx * dx + dy * dy;
  }

  [[nodiscard]] constexpr float dot(const Point& other) const { return x * other.x + y * other.y; }

  [[nodiscard]] constexpr float cross(const Point& other) const {
    return x * other.y - y * other.x;
  }

  [[nodiscard]] bool canNormalize() const {
    return std::isfinite(x) && std::isfinite(y) && (x != 0.0f || y != 0.0f);
  }

  bool normalize() { return setLength(1.0f); }

  bool setNormalize(float px, float py) {
    x = px;
    y = py;
    return setLength(1.0f);
  }

  bool setLength(float len) {
    // Use f64 for the scale computation.
    double xx = x, yy = y;
    double dmag = std::sqrt(xx * xx + yy * yy);
    double dscale = static_cast<double>(len) / dmag;
    x *= static_cast<float>(dscale);
    y *= static_cast<float>(dscale);
    if (!std::isfinite(x) || !std::isfinite(y) || (x == 0.0f && y == 0.0f)) {
      x = 0.0f;
      y = 0.0f;
      return false;
    }
    return true;
  }

  void scale(float factor) {
    x *= factor;
    y *= factor;
  }

  [[nodiscard]] constexpr Point scaled(float factor) const { return Point{x * factor, y * factor}; }

  void rotateCw() {
    float tmp = x;
    x = -y;
    y = tmp;
  }

  void rotateCcw() {
    float tmp = x;
    x = y;
    y = -tmp;
  }

  [[nodiscard]] bool equalsWithinTolerance(const Point& other, float tolerance) const {
    return std::abs(x - other.x) <= tolerance && std::abs(y - other.y) <= tolerance;
  }

  constexpr bool operator==(const Point&) const = default;
};

[[nodiscard]] inline constexpr Point operator-(const Point& a, const Point& b) {
  return Point{a.x - b.x, a.y - b.y};
}

[[nodiscard]] inline constexpr Point operator+(const Point& a, const Point& b) {
  return Point{a.x + b.x, a.y + b.y};
}

[[nodiscard]] inline constexpr Point operator-(const Point& a) { return Point{-a.x, -a.y}; }

}  // namespace tiny_skia
