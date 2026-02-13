#include <array>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/path64/Cubic64.h"
#include "tiny_skia/path64/LineCubicIntersections.h"

TEST(LineCubicIntersectionsTest, HorizontalIntersectFindsExpectedSingleRoot) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(0.3333333333333333, 1.0 / 3.0),
      tiny_skia::Point64::fromXy(0.6666666666666666, 2.0 / 3.0),
      tiny_skia::Point64::fromXy(1.0, 1.0),
  });

  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::line_cubic_intersections::horizontalIntersect(cubic, 0.5, roots);
  EXPECT_EQ(count, 1u);
  EXPECT_DOUBLE_EQ(roots[0], 0.5);
}

TEST(LineCubicIntersectionsTest, VerticalIntersectFindsExpectedSingleRoot) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(1.0 / 3.0, 0.0),
      tiny_skia::Point64::fromXy(2.0 / 3.0, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
  });

  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::line_cubic_intersections::verticalIntersect(cubic, 0.5, roots);
  EXPECT_EQ(count, 1u);
  EXPECT_DOUBLE_EQ(roots[0], 0.5);
}

TEST(LineCubicIntersectionsTest, VerticalIntersectReturnsZeroForMiss) {
  const auto cubic = tiny_skia::path64::cubic64::Cubic64::create({
      tiny_skia::Point64::fromXy(0.0, 0.0),
      tiny_skia::Point64::fromXy(0.25, 0.0),
      tiny_skia::Point64::fromXy(0.5, 0.0),
      tiny_skia::Point64::fromXy(1.0, 0.0),
  });

  std::array<double, 3> roots{};
  const auto count = tiny_skia::path64::line_cubic_intersections::verticalIntersect(cubic, 2.0, roots);
  EXPECT_EQ(count, 0u);
}
