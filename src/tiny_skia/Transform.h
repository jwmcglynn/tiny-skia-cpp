#pragma once

#include <optional>
#include <span>

namespace tiny_skia {

struct Point;

/// Affine transformation matrix [sx kx tx; ky sy ty; 0 0 1].
class Transform {
 public:
  float sx = 1.0f;
  float kx = 0.0f;
  float ky = 0.0f;
  float sy = 1.0f;
  float tx = 0.0f;
  float ty = 0.0f;

  constexpr Transform() = default;

  [[nodiscard]] static constexpr Transform identity() { return Transform{}; }

  [[nodiscard]] static constexpr Transform fromRow(float sx, float ky, float kx, float sy, float tx,
                                                   float ty) {
    Transform t;
    t.sx = sx;
    t.ky = ky;
    t.kx = kx;
    t.sy = sy;
    t.tx = tx;
    t.ty = ty;
    return t;
  }

  [[nodiscard]] static constexpr Transform fromTranslate(float tx, float ty) {
    return fromRow(1.0f, 0.0f, 0.0f, 1.0f, tx, ty);
  }

  [[nodiscard]] static constexpr Transform fromScale(float sx, float sy) {
    return fromRow(sx, 0.0f, 0.0f, sy, 0.0f, 0.0f);
  }

  [[nodiscard]] bool isFinite() const;
  [[nodiscard]] bool isIdentity() const;
  [[nodiscard]] bool isTranslate() const;
  [[nodiscard]] bool isScaleTranslate() const;
  [[nodiscard]] bool hasScale() const;
  [[nodiscard]] bool hasSkew() const;
  [[nodiscard]] bool hasTranslate() const;

  [[nodiscard]] std::optional<Transform> invert() const;
  [[nodiscard]] Transform preConcat(const Transform& other) const;
  [[nodiscard]] Transform postConcat(const Transform& other) const;
  [[nodiscard]] Transform preScale(float sx, float sy) const;
  [[nodiscard]] Transform postScale(float sx, float sy) const;
  [[nodiscard]] Transform preTranslate(float tx, float ty) const;
  [[nodiscard]] Transform postTranslate(float tx, float ty) const;

  /// Maps an array of points through the transform.
  void mapPoints(std::span<Point> points) const;

  constexpr bool operator==(const Transform&) const = default;
};

}  // namespace tiny_skia
