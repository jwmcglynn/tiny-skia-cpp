#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/Path.h"
#include "tiny_skia/PathBuilder.h"

using ::testing::Optional;

// ---- computeTightBounds tests ----

TEST(PathTest, ComputeTightBoundsOnEmptyPathReturnsNullopt) {
  tiny_skia::Path path;
  EXPECT_EQ(path.computeTightBounds(), std::nullopt);
}

TEST(PathTest, ComputeTightBoundsOnLinePathMatchesBounds) {
  tiny_skia::PathBuilder builder;
  builder.moveTo(10.0f, 20.0f);
  builder.lineTo(30.0f, 40.0f);
  auto path = builder.finish();
  ASSERT_TRUE(path.has_value());

  auto tight = path->computeTightBounds();
  ASSERT_THAT(tight, Optional(testing::_));
  EXPECT_FLOAT_EQ(tight->left(), 10.0f);
  EXPECT_FLOAT_EQ(tight->top(), 20.0f);
  EXPECT_FLOAT_EQ(tight->right(), 30.0f);
  EXPECT_FLOAT_EQ(tight->bottom(), 40.0f);

  // For line-only paths, tight bounds == control point bounds.
  auto normal = path->bounds();
  EXPECT_FLOAT_EQ(tight->left(), normal.left());
  EXPECT_FLOAT_EQ(tight->top(), normal.top());
  EXPECT_FLOAT_EQ(tight->right(), normal.right());
  EXPECT_FLOAT_EQ(tight->bottom(), normal.bottom());
}

TEST(PathTest, ComputeTightBoundsOnQuadPathTighterThanBounds) {
  // A quadratic curve whose control point extends above the curve.
  // Move(0,0), Quad(50,100, 100,0): control point at (50,100) but
  // the curve never reaches y=100.
  tiny_skia::PathBuilder builder;
  builder.moveTo(0.0f, 0.0f);
  builder.quadTo(50.0f, 100.0f, 100.0f, 0.0f);
  auto path = builder.finish();
  ASSERT_TRUE(path.has_value());

  auto tight = path->computeTightBounds();
  ASSERT_THAT(tight, Optional(testing::_));
  // The curve's maximum y is 50 (at t=0.5), not 100.
  EXPECT_LT(tight->bottom(), 100.0f);
  EXPECT_NEAR(tight->bottom(), 50.0f, 0.01f);

  // Regular bounds include the control point.
  auto normal = path->bounds();
  EXPECT_FLOAT_EQ(normal.bottom(), 100.0f);
}

TEST(PathTest, ComputeTightBoundsOnCubicPathTighterThanBounds) {
  // Cubic with control points that overshoot.
  // Move(0,0), Cubic(0,100, 100,100, 100,0) — a symmetric arch.
  tiny_skia::PathBuilder builder;
  builder.moveTo(0.0f, 0.0f);
  builder.cubicTo(0.0f, 100.0f, 100.0f, 100.0f, 100.0f, 0.0f);
  auto path = builder.finish();
  ASSERT_TRUE(path.has_value());

  auto tight = path->computeTightBounds();
  ASSERT_THAT(tight, Optional(testing::_));
  // Max y of the curve is 75 (at t=0.5 for this symmetric cubic).
  EXPECT_LT(tight->bottom(), 100.0f);
  EXPECT_NEAR(tight->bottom(), 75.0f, 0.01f);

  // Regular bounds include control points at y=100.
  auto normal = path->bounds();
  EXPECT_FLOAT_EQ(normal.bottom(), 100.0f);
}

// ---- clear tests ----

TEST(PathTest, ClearReturnsEmptyPathBuilder) {
  tiny_skia::PathBuilder builder;
  builder.moveTo(10.0f, 20.0f);
  builder.lineTo(30.0f, 40.0f);
  auto path = builder.finish();
  ASSERT_TRUE(path.has_value());
  ASSERT_FALSE(path->isEmpty());

  auto newBuilder = path->clear();
  EXPECT_TRUE(path->isEmpty());  // Path should be empty after clear.
  EXPECT_TRUE(newBuilder.isEmpty());  // Builder starts empty.

  // The returned builder should be usable.
  newBuilder.moveTo(1.0f, 2.0f);
  newBuilder.lineTo(3.0f, 4.0f);
  auto newPath = newBuilder.finish();
  ASSERT_TRUE(newPath.has_value());
  EXPECT_EQ(newPath->len(), 2u);
}
