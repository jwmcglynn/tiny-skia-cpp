#pragma once

#include "tiny_skia/PathGeometry.h"

namespace tiny_skia::path_geometry::path_rs {

// Wrappers for functions owned by `third_party/tiny-skia/path/src/path_geometry.rs`.

using Conic = ::tiny_skia::path_geometry::Conic;
using AutoConicToQuads = ::tiny_skia::path_geometry::AutoConicToQuads;

inline std::size_t chopQuadAt(const std::array<Point, 3>& src, float t, std::array<Point, 5>& dst) {
  return ::tiny_skia::path_geometry::chopQuadAt(src, t, dst);
}

inline std::size_t chopCubicAt(std::span<const Point> src,
                               std::span<const NormalizedF32Exclusive> tValues,
                               std::span<Point> dst) {
  return ::tiny_skia::path_geometry::chopCubicAt(src, tValues, dst);
}

inline void chopQuadAtT(const Point src[3], NormalizedF32Exclusive t, Point dst[5]) {
  ::tiny_skia::path_geometry::chopQuadAtT(src, t, dst);
}

inline void chopCubicAt2(const Point src[4], NormalizedF32Exclusive t, Point dst[7]) {
  ::tiny_skia::path_geometry::chopCubicAt2(src, t, dst);
}

inline Point evalQuadAt(const Point src[3], NormalizedF32 t) {
  return ::tiny_skia::path_geometry::evalQuadAt(src, t);
}

inline Point evalQuadTangentAt(const Point src[3], NormalizedF32 t) {
  return ::tiny_skia::path_geometry::evalQuadTangentAt(src, t);
}

inline Point evalCubicPosAt(const Point src[4], NormalizedF32 t) {
  return ::tiny_skia::path_geometry::evalCubicPosAt(src, t);
}

inline Point evalCubicTangentAt(const Point src[4], NormalizedF32 t) {
  return ::tiny_skia::path_geometry::evalCubicTangentAt(src, t);
}

inline NormalizedF32 findQuadMaxCurvature(const Point src[3]) {
  return ::tiny_skia::path_geometry::findQuadMaxCurvature(src);
}

inline std::optional<NormalizedF32Exclusive> findQuadExtrema(float a, float b, float c) {
  return ::tiny_skia::path_geometry::findQuadExtrema(a, b, c);
}

inline std::size_t findCubicExtremaT(float a, float b, float c, float d,
                                     NormalizedF32Exclusive tValues[3]) {
  return ::tiny_skia::path_geometry::findCubicExtremaT(a, b, c, d, tValues);
}

inline std::size_t findCubicInflections(const Point src[4], NormalizedF32Exclusive tValues[3]) {
  return ::tiny_skia::path_geometry::findCubicInflections(src, tValues);
}

inline std::size_t findCubicMaxCurvatureTs(const Point src[4], NormalizedF32 tValues[3]) {
  return ::tiny_skia::path_geometry::findCubicMaxCurvatureTs(src, tValues);
}

inline std::optional<NormalizedF32Exclusive> findCubicCusp(const Point src[4]) {
  return ::tiny_skia::path_geometry::findCubicCusp(src);
}

inline std::size_t findUnitQuadRoots(float a, float b, float c, NormalizedF32Exclusive roots[3]) {
  return ::tiny_skia::path_geometry::findUnitQuadRoots(a, b, c, roots);
}

inline std::array<NormalizedF32Exclusive, 3> newTValues() {
  return ::tiny_skia::path_geometry::newTValues();
}

inline std::optional<AutoConicToQuads> autoConicToQuads(Point pt0, Point pt1, Point pt2,
                                                        float weight) {
  return ::tiny_skia::path_geometry::autoConicToQuads(pt0, pt1, pt2, weight);
}

}  // namespace tiny_skia::path_geometry::path_rs
