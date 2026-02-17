#include "tiny_skia/PathGeometry.h"

#include <cmath>
#include <limits>
#include <span>

#include "tiny_skia/Math.h"
#include "tiny_skia/path64/Cubic64.h"
#include "tiny_skia/path64/LineCubicIntersections.h"
#include "tiny_skia/path64/Point64.h"
#include "tiny_skia/path64/Quad64.h"
#include "tiny_skia/pipeline/Mod.h"

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

// --- New functions needed by stroker / dash ---

namespace {

// float interp helper
float interpF(float a, float b, float t) { return a + (b - a) * t; }

Point interpPt(Point a, Point b, float t) {
  return Point::fromXy(interpF(a.x, b.x, t), interpF(a.y, b.y, t));
}

// Quad coefficient representation
struct QuadCoeff {
  float ax, ay;  // coefficient a
  float bx, by;  // coefficient b
  float cx, cy;  // coefficient c

  static QuadCoeff fromPoints(const Point src[3]) {
    float cx = src[0].x;
    float cy = src[0].y;
    float bx = 2.0f * (src[1].x - cx);
    float by = 2.0f * (src[1].y - cy);
    float ax = src[2].x - 2.0f * src[1].x + cx;
    float ay = src[2].y - 2.0f * src[1].y + cy;
    return {ax, ay, bx, by, cx, cy};
  }

  Point eval(float t) const {
    return Point::fromXy((ax * t + bx) * t + cx, (ay * t + by) * t + cy);
  }
};

// Cubic coefficient representation
struct CubicCoeff {
  float ax, ay, bx, by, cx, cy, dx, dy;

  static CubicCoeff fromPoints(const Point src[4]) {
    float p0x = src[0].x, p0y = src[0].y;
    float p1x = src[1].x, p1y = src[1].y;
    float p2x = src[2].x, p2y = src[2].y;
    float p3x = src[3].x, p3y = src[3].y;

    return {
        p3x + 3.0f * (p1x - p2x) - p0x,
        p3y + 3.0f * (p1y - p2y) - p0y,
        3.0f * (p2x - 2.0f * p1x + p0x),
        3.0f * (p2y - 2.0f * p1y + p0y),
        3.0f * (p1x - p0x),
        3.0f * (p1y - p0y),
        p0x,
        p0y,
    };
  }

  Point eval(float t) const {
    return Point::fromXy(((ax * t + bx) * t + cx) * t + dx,
                         ((ay * t + by) * t + cy) * t + dy);
  }
};

Point evalCubicDerivative(const Point src[4], NormalizedF32 t) {
  // Derivative: 3[(b-a) + 2(a-2b+c)t + (d+3(b-c)-a)t^2]
  float p0x = src[0].x, p0y = src[0].y;
  float p1x = src[1].x, p1y = src[1].y;
  float p2x = src[2].x, p2y = src[2].y;
  float p3x = src[3].x, p3y = src[3].y;

  // Use QuadCoeff for derivative
  float ax = p3x + 3.0f * (p1x - p2x) - p0x;
  float ay = p3y + 3.0f * (p1y - p2y) - p0y;
  float bx = 2.0f * (p2x - 2.0f * p1x + p0x);
  float by = 2.0f * (p2y - 2.0f * p1y + p0y);
  float cx = p1x - p0x;
  float cy = p1y - p0y;

  float tv = t.get();
  return Point::fromXy((ax * tv + bx) * tv + cx, (ay * tv + by) * tv + cy);
}

std::optional<NormalizedF32Exclusive> validUnitDivideF32(float numer,
                                                         float denom) {
  if (numer < 0.0f) {
    numer = -numer;
    denom = -denom;
  }
  if (denom == 0.0f || numer == 0.0f || numer >= denom) {
    return std::nullopt;
  }
  float r = numer / denom;
  return NormalizedF32Exclusive::create(r);
}

// formulate_f1_dot_f2 (float variant for cubic max curvature)
std::array<float, 4> formulateF1DotF2f(const float src[4]) {
  float a = src[1] - src[0];
  float b = src[2] - 2.0f * src[1] + src[0];
  float c = src[3] + 3.0f * (src[1] - src[2]) - src[0];
  return {c * c, 3.0f * b * c, 2.0f * b * b + c * a, a * b};
}

constexpr float kFloatPi = 3.14159265f;

float scalarCubeRoot(float x) { return std::pow(std::abs(x), 1.0f / 3.0f) * (x < 0 ? -1.0f : 1.0f); }

void sortArray3(NormalizedF32 arr[3]) {
  if (arr[0] > arr[1]) std::swap(arr[0], arr[1]);
  if (arr[1] > arr[2]) std::swap(arr[1], arr[2]);
  if (arr[0] > arr[1]) std::swap(arr[0], arr[1]);
}

std::size_t collapseDuplicates3(NormalizedF32 arr[3]) {
  std::size_t len = 3;
  if (arr[1] == arr[2]) len = 2;
  if (arr[0] == arr[1]) len = 1;
  return len;
}

std::size_t solveCubicPoly(const float coeff[4], NormalizedF32 tValues[3]) {
  if (isNearlyZero(coeff[0])) {
    NormalizedF32Exclusive tmpT[3] = {
        NormalizedF32Exclusive::HALF, NormalizedF32Exclusive::HALF,
        NormalizedF32Exclusive::HALF};
    auto count =
        findUnitQuadRoots(coeff[1], coeff[2], coeff[3], tmpT);
    for (std::size_t i = 0; i < count; ++i) {
      tValues[i] = tmpT[i].toNormalized();
    }
    return count;
  }

  float inva = 1.0f / coeff[0];
  float a = coeff[1] * inva;
  float b = coeff[2] * inva;
  float c = coeff[3] * inva;

  float q = (a * a - b * 3.0f) / 9.0f;
  float r =
      (2.0f * a * a * a - 9.0f * a * b + 27.0f * c) / 54.0f;

  float q3 = q * q * q;
  float r2MinusQ3 = r * r - q3;
  float adiv3 = a / 3.0f;

  if (r2MinusQ3 < 0.0f) {
    float qSqrt = std::sqrt(q);
    float div = r / (q * qSqrt);
    // clamp to [-1, 1]
    if (div < -1.0f) div = -1.0f;
    if (div > 1.0f) div = 1.0f;
    float theta = std::acos(div);
    float neg2RootQ = -2.0f * qSqrt;

    tValues[0] =
        NormalizedF32::newClamped(neg2RootQ * std::cos(theta / 3.0f) - adiv3);
    tValues[1] = NormalizedF32::newClamped(
        neg2RootQ * std::cos((theta + 2.0f * kFloatPi) / 3.0f) - adiv3);
    tValues[2] = NormalizedF32::newClamped(
        neg2RootQ * std::cos((theta - 2.0f * kFloatPi) / 3.0f) - adiv3);

    sortArray3(tValues);
    return collapseDuplicates3(tValues);
  } else {
    float absR = std::abs(r);
    float av = absR + std::sqrt(r2MinusQ3);
    av = scalarCubeRoot(av);
    if (r > 0.0f) av = -av;
    if (av != 0.0f) av += q / av;
    tValues[0] = NormalizedF32::newClamped(av - adiv3);
    return 1;
  }
}

bool onSameSide(const Point src[4], std::size_t testIndex,
                std::size_t lineIndex) {
  Point origin = src[lineIndex];
  Point line = src[lineIndex + 1] - origin;
  float crosses[2];
  for (std::size_t i = 0; i < 2; ++i) {
    Point testLine = src[testIndex + i] - origin;
    crosses[i] = line.cross(testLine);
  }
  return crosses[0] * crosses[1] >= 0.0f;
}

float calcCubicPrecision(const Point src[4]) {
  return (src[1].distanceToSqd(src[0]) + src[2].distanceToSqd(src[1]) +
          src[3].distanceToSqd(src[2])) *
         1e-8f;
}

}  // namespace

void chopQuadAtT(const Point src[3], NormalizedF32Exclusive t,
                 Point dst[5]) {
  float tv = t.get();
  Point p01 = interpPt(src[0], src[1], tv);
  Point p12 = interpPt(src[1], src[2], tv);
  Point p012 = interpPt(p01, p12, tv);
  dst[0] = src[0];
  dst[1] = p01;
  dst[2] = p012;
  dst[3] = p12;
  dst[4] = src[2];
}

void chopCubicAt2(const Point src[4], NormalizedF32Exclusive t,
                  Point dst[7]) {
  float tv = t.get();
  Point ab = interpPt(src[0], src[1], tv);
  Point bc = interpPt(src[1], src[2], tv);
  Point cd = interpPt(src[2], src[3], tv);
  Point abc = interpPt(ab, bc, tv);
  Point bcd = interpPt(bc, cd, tv);
  Point abcd = interpPt(abc, bcd, tv);
  dst[0] = src[0];
  dst[1] = ab;
  dst[2] = abc;
  dst[3] = abcd;
  dst[4] = bcd;
  dst[5] = cd;
  dst[6] = src[3];
}

Point evalQuadAt(const Point src[3], NormalizedF32 t) {
  return QuadCoeff::fromPoints(src).eval(t.get());
}

Point evalQuadTangentAt(const Point src[3], NormalizedF32 t) {
  if ((t == NormalizedF32::ZERO && src[0] == src[1]) ||
      (t == NormalizedF32::ONE && src[1] == src[2])) {
    return src[2] - src[0];
  }

  float bx = src[1].x - src[0].x;
  float by = src[1].y - src[0].y;
  float ax = src[2].x - src[1].x - bx;
  float ay = src[2].y - src[1].y - by;
  float tv = t.get();
  float tx = ax * tv + bx;
  float ty = ay * tv + by;
  return Point::fromXy(tx + tx, ty + ty);
}

Point evalCubicPosAt(const Point src[4], NormalizedF32 t) {
  return CubicCoeff::fromPoints(src).eval(t.get());
}

Point evalCubicTangentAt(const Point src[4], NormalizedF32 t) {
  if ((t.get() == 0.0f && src[0] == src[1]) ||
      (t.get() == 1.0f && src[2] == src[3])) {
    Point tangent;
    if (t.get() == 0.0f) {
      tangent = src[2] - src[0];
    } else {
      tangent = src[3] - src[1];
    }
    if (tangent.x == 0.0f && tangent.y == 0.0f) {
      tangent = src[3] - src[0];
    }
    return tangent;
  }
  return evalCubicDerivative(src, t);
}

NormalizedF32 findQuadMaxCurvature(const Point src[3]) {
  float ax = src[1].x - src[0].x;
  float ay = src[1].y - src[0].y;
  float bx = src[0].x - src[1].x - src[1].x + src[2].x;
  float by = src[0].y - src[1].y - src[1].y + src[2].y;

  float numer = -(ax * bx + ay * by);
  float denom = bx * bx + by * by;
  if (denom < 0.0f) {
    numer = -numer;
    denom = -denom;
  }
  if (numer <= 0.0f) return NormalizedF32::ZERO;
  if (numer >= denom) return NormalizedF32::ONE;

  float t = numer / denom;
  auto result = NormalizedF32::create(t);
  return result.value_or(NormalizedF32::ZERO);
}

std::optional<NormalizedF32Exclusive> findQuadExtrema(float a, float b,
                                                      float c) {
  return validUnitDivideF32(a - b, a - b - b + c);
}

std::size_t findCubicExtremaT(float a, float b, float c, float d,
                              NormalizedF32Exclusive tValues[3]) {
  // We divide A, B, C by 3 to simplify (matches Rust find_cubic_extrema).
  const float aa = d - a + 3.0f * (b - c);
  const float bb = 2.0f * (a - b - b + c);
  const float cc = b - a;
  return findUnitQuadRoots(aa, bb, cc, tValues);
}

std::size_t findCubicInflections(const Point src[4],
                                 NormalizedF32Exclusive tValues[3]) {
  float ax = src[1].x - src[0].x;
  float ay = src[1].y - src[0].y;
  float bx = src[2].x - 2.0f * src[1].x + src[0].x;
  float by = src[2].y - 2.0f * src[1].y + src[0].y;
  float cx = src[3].x + 3.0f * (src[1].x - src[2].x) - src[0].x;
  float cy = src[3].y + 3.0f * (src[1].y - src[2].y) - src[0].y;

  return findUnitQuadRoots(bx * cy - by * cx, ax * cy - ay * cx,
                           ax * by - ay * bx, tValues);
}

std::size_t findCubicMaxCurvatureTs(const Point src[4],
                                     NormalizedF32 tValues[3]) {
  float srcX[4] = {src[0].x, src[1].x, src[2].x, src[3].x};
  float srcY[4] = {src[0].y, src[1].y, src[2].y, src[3].y};
  auto coeffX = formulateF1DotF2f(srcX);
  auto coeffY = formulateF1DotF2f(srcY);
  float coeff[4];
  for (int i = 0; i < 4; ++i) {
    coeff[i] = coeffX[i] + coeffY[i];
  }
  return solveCubicPoly(coeff, tValues);
}

std::optional<NormalizedF32Exclusive> findCubicCusp(const Point src[4]) {
  if (src[0] == src[1]) return std::nullopt;
  if (src[2] == src[3]) return std::nullopt;

  if (onSameSide(src, 0, 2) || onSameSide(src, 2, 0)) {
    return std::nullopt;
  }

  NormalizedF32 tVals[3];
  auto count = findCubicMaxCurvatureTs(src, tVals);
  for (std::size_t i = 0; i < count; ++i) {
    if (tVals[i].get() <= 0.0f || tVals[i].get() >= 1.0f) continue;
    auto dPt = evalCubicDerivative(src, tVals[i]);
    float dPtMag = dPt.lengthSqd();
    float precision = calcCubicPrecision(src);
    if (dPtMag < precision) {
      return NormalizedF32Exclusive::newBounded(tVals[i].get());
    }
  }
  return std::nullopt;
}

std::size_t findUnitQuadRoots(float a, float b, float c,
                               NormalizedF32Exclusive roots[3]) {
  if (a == 0.0f) {
    auto r = validUnitDivideF32(-c, b);
    if (r.has_value()) {
      roots[0] = *r;
      return 1;
    }
    return 0;
  }

  double dr = static_cast<double>(b) * static_cast<double>(b) -
              4.0 * static_cast<double>(a) * static_cast<double>(c);
  if (dr < 0.0) return 0;
  dr = std::sqrt(dr);
  float r = static_cast<float>(dr);
  if (!std::isfinite(r)) return 0;

  float q = (b < 0.0f) ? -(b - r) / 2.0f : -(b + r) / 2.0f;

  std::size_t count = 0;
  if (auto rv = validUnitDivideF32(q, a)) {
    roots[count++] = *rv;
  }
  if (auto rv = validUnitDivideF32(c, q)) {
    roots[count++] = *rv;
  }

  if (count == 2) {
    if (roots[0].get() > roots[1].get()) {
      std::swap(roots[0], roots[1]);
    } else if (roots[0] == roots[1]) {
      count = 1;
    }
  }
  return count;
}

// Conic implementation

Conic Conic::create(Point p0, Point p1, Point p2, float w) {
  Conic c;
  c.points[0] = p0;
  c.points[1] = p1;
  c.points[2] = p2;
  c.weight = w;
  return c;
}

Conic Conic::fromPoints(const Point pts[], float w) {
  return create(pts[0], pts[1], pts[2], w);
}

void Conic::chop(Conic dst[2]) const {
  float scale = 1.0f / (1.0f + weight);
  float newW = std::sqrt(0.5f + weight * 0.5f);

  Point wp1 = points[1].scaled(weight);
  Point m = Point::fromXy((points[0].x + 2.0f * wp1.x + points[2].x) * scale *
                               0.5f,
                           (points[0].y + 2.0f * wp1.y + points[2].y) * scale *
                               0.5f);

  dst[0].points[0] = points[0];
  dst[0].points[1] = Point::fromXy((points[0].x + wp1.x) * scale,
                                    (points[0].y + wp1.y) * scale);
  dst[0].points[2] = m;
  dst[0].weight = newW;

  dst[1].points[0] = m;
  dst[1].points[1] = Point::fromXy((wp1.x + points[2].x) * scale,
                                    (wp1.y + points[2].y) * scale);
  dst[1].points[2] = points[2];
  dst[1].weight = newW;
}

std::optional<std::uint8_t> Conic::computeQuadPow2(float tolerance) const {
  if (tolerance < 0.0f || !std::isfinite(tolerance)) return std::nullopt;

  if (!(weight > 0.0f)) return std::nullopt;
  if (!std::isfinite(weight)) return std::nullopt;

  float a = weight - 1.0f;
  float k = a / (4.0f * (2.0f + a));
  float x = k * (points[0].x - 2.0f * points[1].x + points[2].x);
  float y = k * (points[0].y - 2.0f * points[1].y + points[2].y);

  float error = std::sqrt(x * x + y * y);
  std::uint8_t pow2 = 0;
  for (; pow2 < 5; ++pow2) {
    if (error <= tolerance) break;
    error *= 0.25f;
  }
  return pow2;
}

std::uint8_t Conic::chopIntoQuadsPow2(std::uint8_t pow2,
                                       Point dst[]) const {
  if (pow2 == 0) {
    dst[0] = points[0];
    dst[1] = points[1];
    dst[2] = points[2];
    return 1;
  }

  Conic src = *this;
  // Max pow2 is 5, so max conics is 32, needing 33 Conics for temp storage
  Conic tmp[33];
  tmp[0] = src;

  for (std::uint8_t i = 0; i < pow2; ++i) {
    std::size_t count = static_cast<std::size_t>(1) << i;
    for (std::size_t j = count; j > 0; --j) {
      tmp[j - 1].chop(&tmp[2 * (j - 1)]);
    }
  }

  std::size_t quadCount = static_cast<std::size_t>(1) << pow2;
  dst[0] = tmp[0].points[0];
  for (std::size_t i = 0; i < quadCount; ++i) {
    dst[2 * i + 1] = tmp[i].points[1];
    dst[2 * i + 2] = tmp[i].points[2];
  }

  return static_cast<std::uint8_t>(quadCount);
}

std::optional<std::span<const Conic>> Conic::buildUnitArc(
    Point uStart, Point uStop, PathDirection dir,
    const Transform& userTransform, Conic dst[5]) {
  float x = uStart.dot(uStop);
  float y = uStart.cross(uStop);
  float absY = std::abs(y);

  constexpr float kNearlyZero = kScalarNearlyZero;

  if (absY <= kNearlyZero && x > 0.0f &&
      ((y >= 0.0f && dir == PathDirection::CW) ||
       (y <= 0.0f && dir == PathDirection::CCW))) {
    return std::nullopt;
  }

  if (dir == PathDirection::CCW) {
    y = -y;
  }

  std::size_t quadrant = 0;
  if (y == 0.0f) {
    quadrant = 2;
  } else if (x == 0.0f) {
    quadrant = (y > 0.0f) ? 1 : 3;
  } else {
    if (y < 0.0f) quadrant += 2;
    if ((x < 0.0f) != (y < 0.0f)) quadrant += 1;
  }

  const Point quadrantPoints[8] = {
      Point::fromXy(1.0f, 0.0f),  Point::fromXy(1.0f, 1.0f),
      Point::fromXy(0.0f, 1.0f),  Point::fromXy(-1.0f, 1.0f),
      Point::fromXy(-1.0f, 0.0f), Point::fromXy(-1.0f, -1.0f),
      Point::fromXy(0.0f, -1.0f), Point::fromXy(1.0f, -1.0f),
  };

  constexpr float kQuadrantWeight = kScalarRoot2Over2;

  std::size_t conicCount = quadrant;
  for (std::size_t i = 0; i < conicCount; ++i) {
    dst[i] = Conic::fromPoints(&quadrantPoints[i * 2], kQuadrantWeight);
  }

  Point finalPt = Point::fromXy(x, y);
  Point lastQ = quadrantPoints[quadrant * 2];
  float dotVal = lastQ.dot(finalPt);

  if (dotVal < 1.0f) {
    Point offCurve = Point::fromXy(lastQ.x + x, lastQ.y + y);
    float cosThetaOver2 = std::sqrt((1.0f + dotVal) / 2.0f);
    if (cosThetaOver2 > 0.0f) {
      offCurve.setLength(1.0f / cosThetaOver2);
    }
    // Check if lastQ and offCurve are not almost equal
    if (std::abs(lastQ.x - offCurve.x) > kNearlyZero ||
        std::abs(lastQ.y - offCurve.y) > kNearlyZero) {
      dst[conicCount] =
          Conic::create(lastQ, offCurve, finalPt, cosThetaOver2);
      conicCount++;
    }
  }

  // Transform: rotate by uStart, optionally flip for CCW, then apply user
  // transform
  float sinVal = uStart.y;
  float cosVal = uStart.x;

  for (std::size_t i = 0; i < conicCount; ++i) {
    for (auto& pt : dst[i].points) {
      float rx = pt.x * cosVal - pt.y * sinVal;
      float ry = pt.x * sinVal + pt.y * cosVal;
      if (dir == PathDirection::CCW) {
        ry = -ry;
      }
      // Apply userTransform
      pt.x = userTransform.sx * rx + userTransform.kx * ry + userTransform.tx;
      pt.y = userTransform.ky * rx + userTransform.sy * ry + userTransform.ty;
    }
  }

  if (conicCount == 0) return std::nullopt;
  return std::span<const Conic>(dst, conicCount);
}

std::optional<AutoConicToQuads> autoConicToQuads(Point pt0, Point pt1,
                                                  Point pt2, float weight) {
  Conic conic = Conic::create(pt0, pt1, pt2, weight);
  auto pow2 = conic.computeQuadPow2(0.25f);
  if (!pow2.has_value()) return std::nullopt;
  AutoConicToQuads result;
  result.len = conic.chopIntoQuadsPow2(*pow2, result.points);
  return result;
}

}  // namespace tiny_skia::path_geometry
