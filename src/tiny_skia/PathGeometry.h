#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "tiny_skia/Edge.h"
#include "tiny_skia/Scalar.h"

namespace tiny_skia {
/// Direction for adding closed contours.
enum class PathDirection : std::uint8_t { CW, CCW };
struct Transform;
}  // namespace tiny_skia

namespace tiny_skia::path_geometry {

// --- Existing functions (used by edge builder / scan) ---

std::size_t chopQuadAt(std::array<Point, 3> src, double t,
                       std::array<Point, 5>& dst);

std::size_t chopQuadAtXExtrema(std::array<Point, 3> src,
                               std::array<Point, 5>& dst);

std::size_t chopQuadAtYExtrema(std::array<Point, 3> src,
                               std::array<Point, 5>& dst);

std::size_t chopCubicAt(std::span<const Point> src,
                        std::span<const double> tValues,
                        std::span<Point> dst);

std::size_t chopCubicAtXExtrema(std::array<Point, 4> src,
                                std::array<Point, 10>& dst);

std::size_t chopCubicAtYExtrema(std::array<Point, 4> src,
                                std::array<Point, 10>& dst);

std::size_t chopCubicAtMaxCurvature(std::array<Point, 4> src,
                                    std::array<double, 3>& tValues,
                                    std::span<Point> dst);

bool chopMonoQuadAtX(std::array<Point, 3> src, float x, double& t);

bool chopMonoQuadAtY(std::array<Point, 3> src, float y, double& t);

bool chopMonoCubicAtX(std::array<Point, 4> src, float x,
                      std::array<Point, 7>& dst);

bool chopMonoCubicAtY(std::array<Point, 4> src, float y,
                      std::array<Point, 7>& dst);

// --- New functions needed by stroker/dash ---

/// Chop a quad at parameter t. Matches Rust chop_quad_at with NormalizedF32Exclusive.
void chopQuadAtT(const Point src[3], NormalizedF32Exclusive t,
                 Point dst[5]);

/// Chop a cubic at parameter t. Matches Rust chop_cubic_at2 (single t).
void chopCubicAt2(const Point src[4], NormalizedF32Exclusive t,
                  Point dst[7]);

/// Evaluate a quad at parameter t. Matches Rust eval_quad_at.
Point evalQuadAt(const Point src[3], NormalizedF32 t);

/// Evaluate the tangent of a quad at parameter t.
Point evalQuadTangentAt(const Point src[3], NormalizedF32 t);

/// Evaluate a cubic at parameter t. Matches Rust eval_cubic_pos_at.
Point evalCubicPosAt(const Point src[4], NormalizedF32 t);

/// Evaluate the tangent of a cubic at parameter t.
Point evalCubicTangentAt(const Point src[4], NormalizedF32 t);

/// Find the t value of max curvature for a quadratic.
NormalizedF32 findQuadMaxCurvature(const Point src[3]);

/// Find extrema of a quadratic component.
std::optional<NormalizedF32Exclusive> findQuadExtrema(float a, float b,
                                                      float c);

/// Find extrema of a cubic component. Matches Rust find_cubic_extrema.
/// Returns the number of extrema found (0..3), stored in tValues.
std::size_t findCubicExtremaT(float a, float b, float c, float d,
                              NormalizedF32Exclusive tValues[3]);

/// Find t values of cubic inflection points.
std::size_t findCubicInflections(const Point src[4],
                                 NormalizedF32Exclusive tValues[3]);

/// Find t values of cubic max curvature.
std::size_t findCubicMaxCurvatureTs(const Point src[4],
                                     NormalizedF32 tValues[3]);

/// Find a cubic cusp.
std::optional<NormalizedF32Exclusive> findCubicCusp(const Point src[4]);

/// Find roots of a*t^2 + b*t + c = 0 in (0,1).
std::size_t findUnitQuadRoots(float a, float b, float c,
                               NormalizedF32Exclusive roots[3]);

/// Array for t values. Matches Rust new_t_values.
inline std::array<NormalizedF32Exclusive, 3> newTValues() {
  return {NormalizedF32Exclusive::HALF, NormalizedF32Exclusive::HALF,
          NormalizedF32Exclusive::HALF};
}

/// Conic (rational quadratic) representation. Matches Rust Conic.
struct Conic {
  Point points[3] = {};
  float weight = 0.0f;

  static Conic create(Point p0, Point p1, Point p2, float w);
  static Conic fromPoints(const Point pts[], float w);

  std::optional<std::uint8_t> computeQuadPow2(float tolerance) const;
  std::uint8_t chopIntoQuadsPow2(std::uint8_t pow2, Point dst[]) const;
  void chop(Conic dst[2]) const;

  static std::optional<std::span<const Conic>> buildUnitArc(
      Point uStart, Point uStop, PathDirection dir,
      const Transform& userTransform, Conic dst[5]);
};

/// Auto conic to quads converter. Matches Rust AutoConicToQuads.
struct AutoConicToQuads {
  Point points[64] = {};
  std::uint8_t len = 0;  // number of quads
};

std::optional<AutoConicToQuads> autoConicToQuads(Point pt0, Point pt1,
                                                  Point pt2, float weight);

}  // namespace tiny_skia::path_geometry
