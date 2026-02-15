#include <array>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/path64/Cubic64.h"

namespace {

auto Point64Eq(double expectedX, double expectedY) {
  return testing::AllOf(
      testing::Field(&tiny_skia::Point64::x, testing::DoubleEq(expectedX)),
      testing::Field(&tiny_skia::Point64::y, testing::DoubleEq(expectedY)));
}

}  // namespace

TEST(Cubic64Test, AsF64SlicePreservesCoordinates) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 1.0),
      tiny_skia::Point64::fromXy(2.0, 3.0),
      tiny_skia::Point64::fromXy(4.0, 5.0),
      tiny_skia::Point64::fromXy(6.0, 7.0),
  });

  const auto slice = cubic.asF64Slice();
  EXPECT_THAT(
      slice,
      testing::ElementsAre(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0));
}

TEST(Cubic64Test, PointAtTEvaluatesEndpointsAndMidpoint) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
  });

  const auto p0 = cubic.pointAtT(0.0);
  const auto p1 = cubic.pointAtT(1.0);
  const auto p2 = cubic.pointAtT(0.5);
  EXPECT_THAT(p0, Point64Eq(0.0, 0.0));
  EXPECT_THAT(p1, Point64Eq(1.0, 0.0));
  EXPECT_THAT(p2, Point64Eq(0.5, 0.0));
}

TEST(Cubic64Test, CoefficientsFollowExpectedTransform) {
  const std::array<double, 8> src{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
  const auto coefficients = tiny_skia::path64::cubic64::coefficients(src);
  EXPECT_THAT(coefficients, testing::ElementsAre(0.0, 0.0, 6.0, 0.0));
}

TEST(Cubic64Test, RootsValidTAddsEndpointsWhenWithinMargin) {
  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::cubic64::rootsValidT(1.0, -3.0, 2.0, 0.0, roots);
  EXPECT_EQ(count, 2u);
  EXPECT_THAT((std::array<double, 2>{roots[0], roots[1]}),
              testing::UnorderedElementsAre(testing::DoubleEq(0.0), testing::DoubleEq(1.0)));
}

TEST(Cubic64Test, RootsValidTForCubicPolynomial) {
  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::cubic64::rootsValidT(1.0, 0.0, -1.0, 0.0, roots);
  EXPECT_EQ(count, 2u);
  EXPECT_THAT((std::array<double, 2>{roots[0], roots[1]}),
              testing::UnorderedElementsAre(testing::DoubleNear(0.0, 1e-12),
                                          testing::DoubleNear(1.0, 1e-12)));
}

TEST(Cubic64Test, ChopAtUsesSpecialCaseAtMidpoint) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
      tiny_skia::Point64::fromXy(2.0, 0.0),
      tiny_skia::Point64::fromXy(3.0, 0.0),
  });
  const auto pair = cubic.chopAt(0.5);

  EXPECT_THAT(pair.points[3], Point64Eq(1.5, 0.0));
  EXPECT_THAT(pair.points[4], Point64Eq(2.0, 0.0));
  EXPECT_THAT(pair.points[5], Point64Eq(2.5, 0.0));
  EXPECT_THAT(pair.points[6], Point64Eq(3.0, 0.0));
}

TEST(Cubic64Test, SearchRootsFindsMonotonicCrossing) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 1.0),
      tiny_skia::Point64::fromXy(1.0, -1.0),
      tiny_skia::Point64::fromXy(2.0, 1.0 / 3.0),
      tiny_skia::Point64::fromXy(3.0, 1.0),
  });
  std::array<double, 6> extremeTs{};
  std::array<double, 3> roots{};
  const auto extrema = tiny_skia::path64::cubic64::findExtrema(cubic.asF64Slice(), extremeTs);
  const auto count = cubic.searchRoots(extrema, 0.0, tiny_skia::SearchAxis::Y, extremeTs, roots);
  EXPECT_EQ(count, 1u);
  EXPECT_DOUBLE_EQ(roots[0], 0.5);
}

TEST(Cubic64Test, RootsRealReturnsThreeDistinctRoots) {
  std::array<double, 3> roots{};
  const auto count =
      tiny_skia::path64::cubic64::rootsValidT(1.0, -1.5, 0.66, -0.08, roots);
  EXPECT_EQ(count, 3u);
  EXPECT_THAT((std::array<double, 3>{roots[0], roots[1], roots[2]}),
              testing::UnorderedElementsAre(testing::DoubleNear(0.2, 1e-12),
                                          testing::DoubleNear(0.5, 1e-12),
                                          testing::DoubleNear(0.8, 1e-12)));
}

TEST(Cubic64Test, FindExtremaForCubicYEqualsTCubedHasOneStationaryPoint) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 1.0),
  });
  std::array<double, 6> extremaTs{};
  const auto count = tiny_skia::path64::cubic64::findExtrema(cubic.asF64Slice(), extremaTs);
  EXPECT_EQ(count, 1u);
  EXPECT_NEAR(extremaTs[0], 0.0, 1e-12);
}

TEST(Cubic64Test, FindExtremaForStraightLineReturnsNone) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 2.0),
      tiny_skia::Point64::fromXy(2.0, 4.0),
      tiny_skia::Point64::fromXy(3.0, 6.0),
  });
  std::array<double, 6> extremaTs{};
  const auto count = tiny_skia::path64::cubic64::findExtrema(cubic.asF64Slice(), extremaTs);
  EXPECT_EQ(count, 0u);
}

TEST(Cubic64Test, SearchRootsFindsNoRootForFlatCurve) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 1.0),
      tiny_skia::Point64::fromXy(0.5, 1.0),
      tiny_skia::Point64::fromXy(1.0, 1.0),
      tiny_skia::Point64::fromXy(1.5, 1.0),
  });
  std::array<double, 6> extremeTs{};
  std::array<double, 3> roots{};
  const auto extrema = tiny_skia::path64::cubic64::findExtrema(cubic.asF64Slice(), extremeTs);
  const auto count = cubic.searchRoots(extrema, 0.0, tiny_skia::SearchAxis::Y, extremeTs, roots);
  EXPECT_EQ(count, 0u);
}

TEST(Cubic64Test, SearchRootsReturnsNoneWhenCurveStaysAboveAxis) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 1.0),
      tiny_skia::Point64::fromXy(1.0, 1.0),
      tiny_skia::Point64::fromXy(2.0, 1.0),
      tiny_skia::Point64::fromXy(3.0, 1.0),
  });
  std::array<double, 6> extremeTs{};
  std::array<double, 3> roots{};
  const auto extrema = tiny_skia::path64::cubic64::findExtrema(cubic.asF64Slice(), extremeTs);
  const auto count = cubic.searchRoots(extrema, 0.0, tiny_skia::SearchAxis::Y, extremeTs, roots);
  EXPECT_EQ(count, 0u);
}

TEST(Cubic64Test, RootsValidTClampsNearOne) {
  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::cubic64::rootsValidT(0.0,
                                                           1.0,
                                                           -3.000004,
                                                           2.000008,
                                                           roots);
  EXPECT_EQ(count, 1u);
  EXPECT_NEAR(roots[0], 1.0, 1e-12);
}

TEST(Cubic64Test, RootsValidTClampsNearZero) {
  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::cubic64::rootsValidT(0.0, 1.0, -1.999996, -0.000008, roots);
  EXPECT_EQ(count, 1u);
  EXPECT_NEAR(roots[0], 0.0, 1e-12);
}

TEST(Cubic64Test, FindInflectionsForCollinearLineIncludesEndpointT) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 1.0),
      tiny_skia::Point64::fromXy(2.0, 2.0),
      tiny_skia::Point64::fromXy(3.0, 3.0),
  });
  std::array<double, 6> extremaTs{};
  auto extremaSpan = std::span<double>(extremaTs.begin(), extremaTs.end());
  const auto count = cubic.findInflections(extremaSpan);

  EXPECT_EQ(count, 1u);
  EXPECT_DOUBLE_EQ(extremaTs[0], 0.0);
}

TEST(Cubic64Test, FindInflectionsFindsInternalInflectionAtHalf) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0 / 3.0, 0.0),
      tiny_skia::Point64::fromXy(2.0 / 3.0, 0.6),
      tiny_skia::Point64::fromXy(1.0, 1.0),
  });
  std::array<double, 6> extremaTs{};
  auto extremaSpan = std::span<double>(extremaTs.begin(), extremaTs.end());
  const auto count = cubic.findInflections(extremaSpan);

  EXPECT_EQ(count, 1u);
  EXPECT_DOUBLE_EQ(extremaTs[0], 0.5);
}
