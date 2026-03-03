// Copyright 2020 Yevhenii Reizner (Rust original)
// Copyright 2025 tiny-skia-cpp contributors (C++ port)
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cmath>

namespace tiny_skia {

// Right now, there are no visible benefits of using SIMD for f32x2/f32x4.
// So we don't. These are simple array wrappers matching Rust's
// tiny-skia-path/src/f32x2_t.rs and f32x4_t.rs.

// --- Internal helpers (NaN-ignoring min/max, matching Rust pmin/pmax) ---

namespace path_vec_detail {

// A faster and more forgiving f32 max: unlike std, we do not care about NaN.
// If a < b, returns b; otherwise returns a.
inline constexpr float pmax(float a, float b) { return a < b ? b : a; }

// A faster and more forgiving f32 min: unlike std, we do not care about NaN.
// If b < a, returns b; otherwise returns a.
inline constexpr float pmin(float a, float b) { return b < a ? b : a; }

}  // namespace path_vec_detail

/// A pair of f32 numbers. Matches Rust `f32x2`.
struct F32x2 {
  float x = 0.0f;
  float y = 0.0f;

  /// Creates a new pair (matches Rust `f32x2::new`).
  [[nodiscard]] static constexpr F32x2 from(float px, float py) {
    return F32x2{px, py};
  }

  /// Creates a new pair from a single value (matches Rust `f32x2::splat`).
  [[nodiscard]] static constexpr F32x2 splat(float v) {
    return F32x2{v, v};
  }

  /// Returns the element-wise absolute value.
  [[nodiscard]] F32x2 abs() const {
    return F32x2{std::abs(x), std::abs(y)};
  }

  /// Returns the element-wise minimum (NaN-ignoring, matches Rust pmin).
  [[nodiscard]] constexpr F32x2 min(F32x2 other) const {
    return F32x2{path_vec_detail::pmin(x, other.x),
                 path_vec_detail::pmin(y, other.y)};
  }

  /// Returns the element-wise maximum (NaN-ignoring, matches Rust pmax).
  [[nodiscard]] constexpr F32x2 max(F32x2 other) const {
    return F32x2{path_vec_detail::pmax(x, other.x),
                 path_vec_detail::pmax(y, other.y)};
  }

  /// Returns the maximum of both components.
  [[nodiscard]] constexpr float maxComponent() const {
    return path_vec_detail::pmax(x, y);
  }

  constexpr bool operator==(const F32x2&) const = default;
};

[[nodiscard]] inline constexpr F32x2 operator+(F32x2 a, F32x2 b) {
  return F32x2{a.x + b.x, a.y + b.y};
}

[[nodiscard]] inline constexpr F32x2 operator-(F32x2 a, F32x2 b) {
  return F32x2{a.x - b.x, a.y - b.y};
}

[[nodiscard]] inline constexpr F32x2 operator*(F32x2 a, F32x2 b) {
  return F32x2{a.x * b.x, a.y * b.y};
}

[[nodiscard]] inline constexpr F32x2 operator/(F32x2 a, F32x2 b) {
  return F32x2{a.x / b.x, a.y / b.y};
}

/// A quartet of f32 numbers. Matches Rust `f32x4`.
struct F32x4 {
  float a = 0.0f;
  float b = 0.0f;
  float c = 0.0f;
  float d = 0.0f;

  /// Creates a new quartet.
  [[nodiscard]] static constexpr F32x4 from(float pa, float pb, float pc,
                                             float pd) {
    return F32x4{pa, pb, pc, pd};
  }

  /// Returns the element-wise maximum (matches Rust f32::max semantics).
  [[nodiscard]] F32x4 max(F32x4 rhs) const {
    return F32x4{std::fmax(a, rhs.a), std::fmax(b, rhs.b),
                 std::fmax(c, rhs.c), std::fmax(d, rhs.d)};
  }

  /// Returns the element-wise minimum (matches Rust f32::min semantics).
  [[nodiscard]] F32x4 min(F32x4 rhs) const {
    return F32x4{std::fmin(a, rhs.a), std::fmin(b, rhs.b),
                 std::fmin(c, rhs.c), std::fmin(d, rhs.d)};
  }

  constexpr F32x4& operator+=(F32x4 rhs) {
    a += rhs.a;
    b += rhs.b;
    c += rhs.c;
    d += rhs.d;
    return *this;
  }

  constexpr F32x4& operator*=(F32x4 rhs) {
    a *= rhs.a;
    b *= rhs.b;
    c *= rhs.c;
    d *= rhs.d;
    return *this;
  }

  constexpr bool operator==(const F32x4&) const = default;
};

[[nodiscard]] inline constexpr F32x4 operator+(F32x4 lhs, F32x4 rhs) {
  return F32x4{lhs.a + rhs.a, lhs.b + rhs.b, lhs.c + rhs.c, lhs.d + rhs.d};
}

[[nodiscard]] inline constexpr F32x4 operator-(F32x4 lhs, F32x4 rhs) {
  return F32x4{lhs.a - rhs.a, lhs.b - rhs.b, lhs.c - rhs.c, lhs.d - rhs.d};
}

[[nodiscard]] inline constexpr F32x4 operator*(F32x4 lhs, F32x4 rhs) {
  return F32x4{lhs.a * rhs.a, lhs.b * rhs.b, lhs.c * rhs.c, lhs.d * rhs.d};
}

}  // namespace tiny_skia
