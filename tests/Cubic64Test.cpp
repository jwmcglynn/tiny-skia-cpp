#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/path64/Cubic64.h"

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
  EXPECT_DOUBLE_EQ(p0.x, 0.0);
  EXPECT_DOUBLE_EQ(p0.y, 0.0);
  EXPECT_DOUBLE_EQ(p1.x, 1.0);
  EXPECT_DOUBLE_EQ(p1.y, 0.0);
  EXPECT_DOUBLE_EQ(p2.x, 0.5);
  EXPECT_DOUBLE_EQ(p2.y, 0.0);
}

TEST(Cubic64Test, CoefficientsFollowExpectedTransform) {
  const std::array<double, 8> src{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
  const auto coefficients = tiny_skia::path64::cubic64::coefficients(src);
  EXPECT_DOUBLE_EQ(coefficients[0], 0.0);
  EXPECT_DOUBLE_EQ(coefficients[1], 0.0);
  EXPECT_DOUBLE_EQ(coefficients[2], 6.0);
  EXPECT_DOUBLE_EQ(coefficients[3], 0.0);
}

TEST(Cubic64Test, RootsValidTAddsEndpointsWhenWithinMargin) {
  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::cubic64::rootsValidT(1.0, -3.0, 2.0, 0.0, roots);
  EXPECT_EQ(count, 2u);
  EXPECT_THAT((std::array<double, 2>{roots[0], roots[1]}),
              testing::UnorderedElementsAre(testing::DoubleEq(0.0), testing::DoubleEq(1.0)));
}

TEST(Cubic64Test, ChopAtUsesSpecialCaseAtMidpoint) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
      tiny_skia::Point64::fromXy(2.0, 0.0),
      tiny_skia::Point64::fromXy(3.0, 0.0),
  });
  const auto pair = cubic.chopAt(0.5);

  EXPECT_DOUBLE_EQ(pair.points[3].x, 1.5);
  EXPECT_DOUBLE_EQ(pair.points[4].x, 2.0);
  EXPECT_DOUBLE_EQ(pair.points[5].x, 2.5);
  EXPECT_DOUBLE_EQ(pair.points[6].x, 3.0);
}
