#include <array>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/PathGeometry.h"

namespace {

::testing::Matcher<const tiny_skia::Point&> PointEq(float x, float y) {
  using ::testing::AllOf;
  using ::testing::FloatEq;
  using ::testing::Field;

  return AllOf(Field("x", &tiny_skia::Point::x, FloatEq(x)),
               Field("y", &tiny_skia::Point::y, FloatEq(y)));
}

}  // namespace

TEST(PathGeometryTest, ChopQuadAtInterpolatesPoints) {
  const auto src =
      std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f}, {10.0f, 10.0f}, {20.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 5>{};
  const auto count = tiny_skia::path_geometry::chopQuadAt(src, 0.25, dst);

  EXPECT_EQ(count, 1u);
  EXPECT_THAT(std::span<const tiny_skia::Point>(dst.data(), 5),
              testing::ElementsAre(PointEq(0.0f, 0.0f), PointEq(2.5f, 2.5f),
                                   PointEq(5.0f, 3.75f), PointEq(12.5f, 7.5f),
                                   PointEq(20.0f, 0.0f)));
}

TEST(PathGeometryTest, ChopQuadAtXExtremaMonotonicLeavesInputIntact) {
  const auto src = std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f},
                                                   tiny_skia::Point{1.0f, 1.0f},
                                                   tiny_skia::Point{2.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 5>{};
  const auto count = tiny_skia::path_geometry::chopQuadAtXExtrema(src, dst);

  EXPECT_EQ(count, 0u);
  EXPECT_FLOAT_EQ(dst[0].x, src[0].x);
  EXPECT_FLOAT_EQ(dst[2].x, src[2].x);
  EXPECT_FLOAT_EQ(dst[1].y, src[1].y);
}

TEST(PathGeometryTest, ChopQuadAtYExtremaMonotonicLeavesInputIntact) {
  const auto src = std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f},
                                                   tiny_skia::Point{1.0f, 1.0f},
                                                   tiny_skia::Point{2.0f, 2.0f}};
  auto dst = std::array<tiny_skia::Point, 5>{};
  const auto count = tiny_skia::path_geometry::chopQuadAtYExtrema(src, dst);

  EXPECT_EQ(count, 0u);
  EXPECT_FLOAT_EQ(dst[0].x, src[0].x);
  EXPECT_FLOAT_EQ(dst[2].x, src[2].x);
}

TEST(PathGeometryTest, ChopQuadAtYExtremaFlattensPeak) {
  const auto src = std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f},
                                                   tiny_skia::Point{1.0f, 10.0f},
                                                   tiny_skia::Point{2.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 5>{};
  const auto count = tiny_skia::path_geometry::chopQuadAtYExtrema(src, dst);

  EXPECT_EQ(count, 1u);
  EXPECT_NEAR(dst[1].x, 1.0f, 1e-6f);
  EXPECT_FLOAT_EQ(dst[2].x, 1.0f);
  EXPECT_FLOAT_EQ(dst[3].x, 1.0f);
  EXPECT_FLOAT_EQ(dst[4].x, 2.0f);
}

TEST(PathGeometryTest, ChopCubicAtReturnsOriginalForEmptyTValues) {
  const auto src = std::array<tiny_skia::Point, 4>{tiny_skia::Point{0.0f, 0.0f},
                                                   {10.0f, 10.0f},
                                                   {20.0f, 10.0f},
                                                   {30.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 10>{};
  const auto span = std::span<const double>{};
  const auto count =
      tiny_skia::path_geometry::chopCubicAt(src, span, std::span<tiny_skia::Point>(dst));

  EXPECT_EQ(count, 0u);
  EXPECT_THAT(dst[0], PointEq(src[0].x, src[0].y));
  EXPECT_THAT(dst[1], PointEq(src[1].x, src[1].y));
  EXPECT_THAT(dst[3], PointEq(src[3].x, src[3].y));
}

TEST(PathGeometryTest, ChopCubicAtSplitsAtOneCut) {
  const auto src = std::array<tiny_skia::Point, 4>{tiny_skia::Point{0.0f, 0.0f},
                                                   {10.0f, 10.0f},
                                                   {20.0f, 10.0f},
                                                   {30.0f, 0.0f}};
  const auto tValues = std::array<double, 1>{0.5};
  auto dst = std::array<tiny_skia::Point, 10>{};
  const auto count =
      tiny_skia::path_geometry::chopCubicAt(src,
                                            std::span<const double>(tValues.data(),
                                                                   tValues.size()),
                                            std::span<tiny_skia::Point>(dst));

  EXPECT_EQ(count, 1u);
  EXPECT_THAT(dst[0], PointEq(0.0f, 0.0f));
  EXPECT_THAT(dst[3], PointEq(15.0f, 7.5f));
  EXPECT_THAT(dst[6], PointEq(30.0f, 0.0f));
}

TEST(PathGeometryTest, ChopCubicAtYExtremaFlattensMonotonicYForCurve) {
  const auto src = std::array<tiny_skia::Point, 4>{tiny_skia::Point{0.0f, 0.0f},
                                                   tiny_skia::Point{1.0f, 3.0f},
                                                   tiny_skia::Point{2.0f, 3.0f},
                                                   tiny_skia::Point{3.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 10>{};
  const auto count = tiny_skia::path_geometry::chopCubicAtYExtrema(src, dst);

  EXPECT_EQ(count, 1u);
  EXPECT_FLOAT_EQ(dst[2].y, dst[3].y);
  EXPECT_FLOAT_EQ(dst[4].y, dst[3].y);
}

TEST(PathGeometryTest, ChopMonoQuadAtXReturnsFalseWhenNoIntersection) {
  const auto src = std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f},
                                                   {2.0f, 2.0f},
                                                   {4.0f, 4.0f}};
  double t = -1.0;
  const auto found = tiny_skia::path_geometry::chopMonoQuadAtX(src, 123.0f, t);

  EXPECT_FALSE(found);
  EXPECT_DOUBLE_EQ(t, -1.0);
}

TEST(PathGeometryTest, ChopMonoQuadAtYReportsTAndLeavesBounds) {
  const auto src = std::array<tiny_skia::Point, 3>{tiny_skia::Point{0.0f, 0.0f},
                                                   {2.0f, 4.0f},
                                                   {4.0f, 0.0f}};
  double t = -1.0;
  const auto found = tiny_skia::path_geometry::chopMonoQuadAtY(src, 2.0f, t);

  EXPECT_TRUE(found);
  EXPECT_GT(t, 0.0);
  EXPECT_LT(t, 1.0);
}

TEST(PathGeometryTest, ChopMonoCubicAtXFindsAndChopsAtIntercept) {
  const auto src = std::array<tiny_skia::Point, 4>{tiny_skia::Point{0.0f, 0.0f},
                                                   {10.0f, 0.0f},
                                                   {20.0f, 0.0f},
                                                   {30.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 7>{};
  const auto found = tiny_skia::path_geometry::chopMonoCubicAtX(src, 15.0f, dst);

  EXPECT_TRUE(found);
  EXPECT_THAT(dst[0], PointEq(0.0f, 0.0f));
  EXPECT_THAT(dst[6], PointEq(30.0f, 0.0f));
  EXPECT_NEAR(dst[3].x, 15.0f, 1e-5);
}

TEST(PathGeometryTest, ChopMonoCubicAtYReturnsFalseWithoutRoots) {
  const auto src = std::array<tiny_skia::Point, 4>{tiny_skia::Point{0.0f, 0.0f},
                                                   {10.0f, 0.0f},
                                                   {20.0f, 0.0f},
                                                   {30.0f, 0.0f}};
  auto dst = std::array<tiny_skia::Point, 7>{};
  const auto found = tiny_skia::path_geometry::chopMonoCubicAtY(src, 100.0f, dst);

  EXPECT_FALSE(found);
}

TEST(PathGeometryTest, ChopCubicAtMaxCurvatureFiltersEndpointsAndSplits) {
  const auto src = std::array<tiny_skia::Point, 4>{
      tiny_skia::Point{20.0f, 160.0f},
      {20.0001f, 160.0f},
      {160.0f, 20.0f},
      {160.0001f, 20.0f},
  };
  auto tValues = std::array<double, 3>{};
  auto dst = std::array<tiny_skia::Point, 10>{};
  const auto count = tiny_skia::path_geometry::chopCubicAtMaxCurvature(
      src, tValues, std::span<tiny_skia::Point>(dst));

  EXPECT_EQ(count, 2u);
  EXPECT_FLOAT_EQ(tValues[0], 0.5f);
  EXPECT_THAT(dst[0], PointEq(20.0f, 160.0f));
  EXPECT_THAT(dst[6], PointEq(160.0001f, 20.0f));
}

TEST(PathGeometryTest, ChopCubicAtMaxCurvatureNoInteriorRootsReturnsOriginalCurve) {
  const auto src = std::array<tiny_skia::Point, 4>{
      tiny_skia::Point{0.0f, 0.0f},
      {1.0f, 1.0f},
      {2.0f, 2.0f},
      {3.0f, 3.0f},
  };
  auto tValues = std::array<double, 3>{1.0, 2.0, 3.0};
  auto dst = std::array<tiny_skia::Point, 4>{};
  const auto count = tiny_skia::path_geometry::chopCubicAtMaxCurvature(
      src, tValues, std::span<tiny_skia::Point>(dst));

  EXPECT_EQ(count, 1u);
  EXPECT_THAT(std::span<const tiny_skia::Point>(dst.data(), 4),
              testing::ElementsAre(PointEq(src[0].x, src[0].y), PointEq(src[1].x, src[1].y),
                                   PointEq(src[2].x, src[2].y), PointEq(src[3].x, src[3].y)));
  EXPECT_FLOAT_EQ(tValues[0], 1.0);
  EXPECT_FLOAT_EQ(tValues[1], 2.0);
  EXPECT_FLOAT_EQ(tValues[2], 3.0);
}
