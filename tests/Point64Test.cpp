#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/Edge.h"
#include "tiny_skia/path64/Point64.h"

TEST(Point64Test, FromPointAndToPointRoundTrip) {
  const tiny_skia::Point source{1.25f, -3.5f};
  const auto p = tiny_skia::Point64::fromPoint(source);

  EXPECT_DOUBLE_EQ(p.x, 1.25);
  EXPECT_DOUBLE_EQ(p.y, -3.5);
  const auto expected = tiny_skia::Point64::fromXy(1.25, -3.5);
  EXPECT_DOUBLE_EQ(p.x, expected.x);
  EXPECT_DOUBLE_EQ(p.y, expected.y);

  const auto back = p.toPoint();
  EXPECT_FLOAT_EQ(back.x, source.x);
  EXPECT_FLOAT_EQ(back.y, source.y);
}

TEST(Point64Test, SearchAxisCoordinates) {
  const tiny_skia::Point64 p{7.5, -2.5};
  EXPECT_DOUBLE_EQ(p.axisCoord(tiny_skia::SearchAxis::X), 7.5);
  EXPECT_DOUBLE_EQ(p.axisCoord(tiny_skia::SearchAxis::Y), -2.5);
}
