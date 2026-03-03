#pragma once

#include "tiny_skia/PathGeometry.h"

namespace tiny_skia::path_geometry::core_rs {

// Wrappers for functions owned by `third_party/tiny-skia/src/path_geometry.rs`.

inline std::size_t chopQuadAtXExtrema(std::array<Point, 3> src,
                                      std::array<Point, 5>& dst) {
  return ::tiny_skia::path_geometry::chopQuadAtXExtrema(src, dst);
}

inline std::size_t chopQuadAtYExtrema(std::array<Point, 3> src,
                                      std::array<Point, 5>& dst) {
  return ::tiny_skia::path_geometry::chopQuadAtYExtrema(src, dst);
}

inline std::size_t chopCubicAtXExtrema(std::array<Point, 4> src,
                                       std::array<Point, 10>& dst) {
  return ::tiny_skia::path_geometry::chopCubicAtXExtrema(src, dst);
}

inline std::size_t chopCubicAtYExtrema(std::array<Point, 4> src,
                                       std::array<Point, 10>& dst) {
  return ::tiny_skia::path_geometry::chopCubicAtYExtrema(src, dst);
}

inline std::size_t chopCubicAtMaxCurvature(std::array<Point, 4> src,
                                           std::array<NormalizedF32Exclusive, 3>& tValues,
                                           std::span<Point> dst) {
  return ::tiny_skia::path_geometry::chopCubicAtMaxCurvature(src, tValues, dst);
}

inline bool chopMonoQuadAtX(std::array<Point, 3> src, float x, float& t) {
  return ::tiny_skia::path_geometry::chopMonoQuadAtX(src, x, t);
}

inline bool chopMonoQuadAtY(std::array<Point, 3> src, float y, float& t) {
  return ::tiny_skia::path_geometry::chopMonoQuadAtY(src, y, t);
}

inline bool chopMonoCubicAtX(std::array<Point, 4> src, float x,
                             std::array<Point, 7>& dst) {
  return ::tiny_skia::path_geometry::chopMonoCubicAtX(src, x, dst);
}

inline bool chopMonoCubicAtY(std::array<Point, 4> src, float y,
                             std::array<Point, 7>& dst) {
  return ::tiny_skia::path_geometry::chopMonoCubicAtY(src, y, dst);
}

}  // namespace tiny_skia::path_geometry::core_rs
