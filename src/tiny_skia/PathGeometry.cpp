#include "tiny_skia/PathGeometry.h"

#include <cmath>
#include <limits>
#include <span>

#include "tiny_skia/path64/Cubic64.h"
#include "tiny_skia/path64/LineCubicIntersections.h"
#include "tiny_skia/path64/Point64.h"
#include "tiny_skia/path64/Quad64.h"

namespace tiny_skia::path_geometry {

namespace {

bool validUnitDivide(double numerator, double denominator, double& normalized) {
  if (numerator < 0.0) {
    numerator = -numerator;
    denominator = -denominator;
  }

  if (denominator <= 0.0 || numerator == 0.0) {
    return false;
  }
  normalized = numerator / denominator;
  return normalized > 0.0 && normalized < 1.0;
}

bool isNotMonotonic(float a, float b, float c) {
  auto ab = a - b;
  auto bc = b - c;
  if (ab < 0.0f) {
    bc = -bc;
  }
  return ab == 0.0f || bc < 0.0f;
}

std::size_t findCubicExtrema(std::span<const float, 4> src, std::array<double, 3>& extrema) {
  const auto na = static_cast<double>(src[3] - src[0] + 3.0f * (src[1] - src[2]));
  const auto nb = static_cast<double>(2.0f * (src[0] - src[1] - src[1] + src[2]));
  const auto nc = static_cast<double>(src[1] - src[0]);
  return tiny_skia::path64::quad64::rootsValidT(na, nb, nc, extrema);
}

std::array<double, 4> formulateF1DotF2(double p0, double p1, double p2, double p3) {
  const auto a = p1 - p0;
  const auto b = p2 - 2.0 * p1 + p0;
  const auto c = p3 + 3.0 * (p1 - p2) - p0;
  return {c * c, 3.0 * b * c, 2.0 * b * b + c * a, a * b};
}

std::size_t findMaxCurvatureRoots(std::array<Point, 4> src, std::array<double, 3>& t) {
  auto x = formulateF1DotF2(src[0].x, src[1].x, src[2].x, src[3].x);
  const auto y = formulateF1DotF2(src[0].y, src[1].y, src[2].y, src[3].y);
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] += y[i];
  }

  return tiny_skia::path64::cubic64::rootsValidT(x[0], x[1], x[2], x[3], t);
}

void interpCubicAt(std::span<const Point, 4> src, double t, std::span<Point, 7> dst) {
  const auto ab = Point{src[0].x + static_cast<float>((src[1].x - src[0].x) * t),
                       src[0].y + static_cast<float>((src[1].y - src[0].y) * t)};
  const auto bc = Point{src[1].x + static_cast<float>((src[2].x - src[1].x) * t),
                       src[1].y + static_cast<float>((src[2].y - src[1].y) * t)};
  const auto cd = Point{src[2].x + static_cast<float>((src[3].x - src[2].x) * t),
                       src[2].y + static_cast<float>((src[3].y - src[2].y) * t)};
  const auto abc = Point{ab.x + static_cast<float>((bc.x - ab.x) * t),
                        ab.y + static_cast<float>((bc.y - ab.y) * t)};
  const auto bcd = Point{bc.x + static_cast<float>((cd.x - bc.x) * t),
                        bc.y + static_cast<float>((cd.y - bc.y) * t)};
  const auto abcd = Point{abc.x + static_cast<float>((bcd.x - abc.x) * t),
                         abc.y + static_cast<float>((bcd.y - abc.y) * t)};

  dst[0] = src[0];
  dst[1] = ab;
  dst[2] = abc;
  dst[3] = abcd;
  dst[4] = bcd;
  dst[5] = cd;
  dst[6] = src[3];
}

void chopCubicAt2(std::span<const Point, 4> src, double t, std::span<Point> dst) {
  if (dst.size() < 7 || t <= 0.0 || t >= 1.0) {
    std::fill_n(dst.data(), dst.size(), Point{});
    return;
  }

  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      Point64::fromPoint(src[0]),
      Point64::fromPoint(src[1]),
      Point64::fromPoint(src[2]),
      Point64::fromPoint(src[3]),
  });
  const auto pair = cubic.chopAt(t);
  for (std::size_t i = 0; i < pair.points.size(); ++i) {
    dst[i] = pair.points[i].toPoint();
  }
}

bool chopMonoCubicAt(std::array<Point, 4> src,
                     float intercept,
                     bool isVertical,
                     std::array<Point, 7>& dst) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      Point64::fromPoint(src[0]),
      Point64::fromPoint(src[1]),
      Point64::fromPoint(src[2]),
      Point64::fromPoint(src[3]),
  });

  auto roots = std::array<double, 3>{};
  const auto count = isVertical
                         ? tiny_skia::path64::line_cubic_intersections::verticalIntersect(
                               cubic, static_cast<double>(intercept), roots)
                         : tiny_skia::path64::line_cubic_intersections::horizontalIntersect(
                               cubic, static_cast<double>(intercept), roots);
  if (count > 0) {
    chopCubicAt2(std::span<const Point, 4>(src), roots[0], std::span<Point, 7>(dst));
    return true;
  }
  return false;
}

bool chopMonoQuadAt(std::array<Point, 3> src, float intercept, bool isVertical, double& t) {
  const auto c0 = isVertical ? src[0].x : src[0].y;
  const auto c1 = isVertical ? src[1].x : src[1].y;
  const auto c2 = isVertical ? src[2].x : src[2].y;
  const auto a = static_cast<double>(c0 - c1 - c1 + c2);
  const auto b = static_cast<double>(2.0f * (c1 - c0));
  const auto c = static_cast<double>(c0 - intercept);
  auto roots = std::array<double, 3>{};
  const auto count = tiny_skia::path64::quad64::rootsValidT(a, b, c, roots);
  if (count > 0) {
    t = roots[0];
    return true;
  }
  return false;
}

}  // namespace

std::size_t chopQuadAt(std::array<Point, 3> src, double t, std::array<Point, 5>& dst) {
  const auto p01 = Point{src[0].x + static_cast<float>((src[1].x - src[0].x) * t),
                         src[0].y + static_cast<float>((src[1].y - src[0].y) * t)};
  const auto p12 = Point{src[1].x + static_cast<float>((src[2].x - src[1].x) * t),
                         src[1].y + static_cast<float>((src[2].y - src[1].y) * t)};
  const auto p012 = Point{p01.x + static_cast<float>((p12.x - p01.x) * t),
                         p01.y + static_cast<float>((p12.y - p01.y) * t)};

  dst[0] = src[0];
  dst[1] = p01;
  dst[2] = p012;
  dst[3] = p12;
  dst[4] = src[2];
  return 1;
}

std::size_t chopQuadAtXExtrema(std::array<Point, 3> src, std::array<Point, 5>& dst) {
  auto a = src[0].x;
  auto b = src[1].x;
  const auto c = src[2].x;

  if (isNotMonotonic(a, b, c)) {
    const auto numerator = static_cast<double>(a - b);
    const auto denominator = static_cast<double>(a - b - b + c);
    if (!std::isinf(denominator) &&
        std::abs(denominator) >= std::numeric_limits<float>::epsilon()) {
      double t = 0.0;
      if (validUnitDivide(numerator, denominator, t)) {
        chopQuadAt(src, t, dst);
        dst[1].y = dst[2].y;
        dst[3].y = dst[2].y;
        return 1;
      }
    }

    b = std::abs(a - b) < std::abs(b - c) ? a : c;
  }

  dst[0] = src[0];
  dst[1] = Point{b, src[1].y};
  dst[2] = src[2];
  return 0;
}

std::size_t chopQuadAtYExtrema(std::array<Point, 3> src, std::array<Point, 5>& dst) {
  auto a = src[0].y;
  auto b = src[1].y;
  const auto c = src[2].y;

  if (isNotMonotonic(a, b, c)) {
    const auto numerator = static_cast<double>(a - b);
    const auto denominator = static_cast<double>(a - b - b + c);
    if (!std::isinf(denominator) &&
        std::abs(denominator) >= std::numeric_limits<float>::epsilon()) {
      double t = 0.0;
      if (validUnitDivide(numerator, denominator, t)) {
        chopQuadAt(src, t, dst);
        dst[1].x = dst[2].x;
        dst[3].x = dst[2].x;
        return 1;
      }
    }

    b = std::abs(a - b) < std::abs(b - c) ? a : c;
  }

  dst[0] = src[0];
  dst[1] = Point{src[1].x, b};
  dst[2] = src[2];
  return 0;
}

std::size_t chopCubicAt(std::span<const Point> src,
                        std::span<const double> tValues,
                        std::span<Point> dst) {
  if (src.size() != 4 || dst.size() < 4) {
    return 0;
  }

  if (tValues.empty()) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    return 0;
  }

  auto srcWork = std::array<Point, 4>{src[0], src[1], src[2], src[3]};
  std::size_t offset = 0;
  double nextT = tValues[0];
  for (std::size_t i = 0; i < tValues.size(); ++i) {
    auto split = std::array<Point, 7>{};
    interpCubicAt(std::span<const Point, 4>(srcWork), nextT, std::span<Point, 7>(split));
    if (offset + 7 > dst.size()) {
      return tValues.size();
    }
    chopCubicAt2(std::span<const Point, 4>(srcWork), nextT, dst.subspan(offset));

    if (i + 1 == tValues.size()) {
      break;
    }

    offset += 3;
    double normalized = 0.0;
    if (!validUnitDivide(tValues[i + 1] - tValues[i], 1.0 - tValues[i], normalized)) {
      dst[offset + 4] = split[6];
      dst[offset + 5] = split[6];
      dst[offset + 6] = split[6];
      break;
    }
    nextT = normalized;
    srcWork = {split[3], split[4], split[5], split[6]};
  }

  return tValues.size();
}

std::size_t chopCubicAtXExtrema(std::array<Point, 4> src, std::array<Point, 10>& dst) {
  auto tValues = std::array<double, 3>{};
  const auto count =
      findCubicExtrema(std::array<float, 4>{src[0].x, src[1].x, src[2].x, src[3].x}, tValues);
  const auto split = chopCubicAt(std::span<const Point, 4>(src),
                                std::span<const double>(tValues.data(), count),
                                std::span<Point, 10>(dst));
  if (split > 0) {
    dst[2].x = dst[3].x;
    dst[4].x = dst[3].x;
    if (split == 2) {
      dst[5].x = dst[6].x;
      dst[7].x = dst[6].x;
    }
  }
  return split;
}

std::size_t chopCubicAtYExtrema(std::array<Point, 4> src, std::array<Point, 10>& dst) {
  auto tValues = std::array<double, 3>{};
  const auto count =
      findCubicExtrema(std::array<float, 4>{src[0].y, src[1].y, src[2].y, src[3].y}, tValues);
  const auto split = chopCubicAt(std::span<const Point, 4>(src),
                                std::span<const double>(tValues.data(), count),
                                std::span<Point, 10>(dst));
  if (split > 0) {
    dst[2].y = dst[3].y;
    dst[4].y = dst[3].y;
    if (split == 2) {
      dst[5].y = dst[6].y;
      dst[7].y = dst[6].y;
    }
  }
  return split;
}

std::size_t chopCubicAtMaxCurvature(std::array<Point, 4> src,
                                    std::array<double, 3>& tValues,
                                    std::span<Point> dst) {
  if (dst.size() < 4) {
    return 0;
  }

  auto roots = std::array<double, 3>{};
  const auto rootCount = findMaxCurvatureRoots(src, roots);
  std::size_t count = 0;
  for (std::size_t i = 0; i < rootCount; ++i) {
    if (roots[i] > 0.0 && roots[i] < 1.0) {
      tValues[count] = roots[i];
      ++count;
    }
  }

  if (count == 0) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    return 1;
  }

  return 1 + chopCubicAt(std::span<const Point, 4>(src),
                         std::span<const double>(tValues.data(), count),
                         dst);
}

bool chopMonoQuadAtX(std::array<Point, 3> src, float x, double& t) {
  return chopMonoQuadAt(src, x, true, t);
}

bool chopMonoQuadAtY(std::array<Point, 3> src, float y, double& t) {
  return chopMonoQuadAt(src, y, false, t);
}

bool chopMonoCubicAtX(std::array<Point, 4> src, float x, std::array<Point, 7>& dst) {
  return chopMonoCubicAt(src, x, true, dst);
}

bool chopMonoCubicAtY(std::array<Point, 4> src, float y, std::array<Point, 7>& dst) {
  return chopMonoCubicAt(src, y, false, dst);
}

}  // namespace tiny_skia::path_geometry
