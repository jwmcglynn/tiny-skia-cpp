#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"

TEST(GeomTest, ScreenIntRectFromXYWHRejectsInvalidDimensions) {
  EXPECT_FALSE(tiny_skia::ScreenIntRect::fromXYWH(0, 0, 0, 0).has_value());
  EXPECT_FALSE(tiny_skia::ScreenIntRect::fromXYWH(0, 0, 1, 0).has_value());
  EXPECT_FALSE(tiny_skia::ScreenIntRect::fromXYWH(0, 0, 0, 1).has_value());
}

TEST(GeomTest, ScreenIntRectFromXYWHRejectsOverflowAndBounds) {
  const auto max = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
  EXPECT_FALSE(
      tiny_skia::ScreenIntRect::fromXYWH(0, 0, max, std::numeric_limits<std::uint32_t>::max())
          .has_value());
  EXPECT_FALSE(tiny_skia::ScreenIntRect::fromXYWH(max, 0, 1, 1).has_value());
  EXPECT_FALSE(tiny_skia::ScreenIntRect::fromXYWH(0, max, 1, 1).has_value());
}

TEST(GeomTest, ScreenIntRectOperations) {
  auto rOpt = tiny_skia::ScreenIntRect::fromXYWH(1, 2, 3, 4);
  ASSERT_TRUE(rOpt.has_value());
  const auto r = rOpt.value();
  EXPECT_EQ(r.x(), 1u);
  EXPECT_EQ(r.y(), 2u);
  EXPECT_EQ(r.width(), 3u);
  EXPECT_EQ(r.height(), 4u);
  EXPECT_EQ(r.right(), 4u);
  EXPECT_EQ(r.bottom(), 6u);
  EXPECT_TRUE(r.contains(r));
}

TEST(GeomTest, IntSizeAndRectConversions) {
  const auto sizeOpt = tiny_skia::IntSize::fromWh(3, 4);
  ASSERT_TRUE(sizeOpt.has_value());

  const auto screen = sizeOpt.value().toScreenIntRect(1, 2);
  EXPECT_EQ(screen.x(), 1u);
  EXPECT_EQ(screen.y(), 2u);
  EXPECT_EQ(screen.width(), 3u);
  EXPECT_EQ(screen.height(), 4u);

  const auto rectOpt =
      tiny_skia::IntRect::fromXYWH(10, 20, 3, 4);
  ASSERT_TRUE(rectOpt.has_value());
  const auto rect = rectOpt.value();
  const auto converted = tiny_skia::intRectToScreen(rect);
  ASSERT_TRUE(converted.has_value());
  const auto convertedValue = converted.value();
  EXPECT_EQ(convertedValue.x(), 10u);
  EXPECT_EQ(convertedValue.y(), 20u);
  EXPECT_EQ(convertedValue.width(), 3u);
  EXPECT_EQ(convertedValue.height(), 4u);
}

TEST(GeomTest, IntSizeFromWhRejectsZero) {
  EXPECT_FALSE(tiny_skia::IntSize::fromWh(0, 1).has_value());
  EXPECT_FALSE(tiny_skia::IntSize::fromWh(1, 0).has_value());
}

TEST(GeomTest, IntRectFromXYWHRejectsInvalidInputs) {
  EXPECT_FALSE(tiny_skia::IntRect::fromXYWH(-1, 0, 1, 1).has_value());
  EXPECT_FALSE(tiny_skia::IntRect::fromXYWH(0, -1, 1, 1).has_value());
  EXPECT_FALSE(tiny_skia::IntRect::fromXYWH(0, 0, 0, 1).has_value());
  EXPECT_FALSE(tiny_skia::IntRect::fromXYWH(0, 0, 1, 0).has_value());
}

TEST(GeomTest, IntRectToScreenIntRectReturnsExpected) {
  const auto rectOpt = tiny_skia::IntRect::fromXYWH(10, 20, 1, 1);
  ASSERT_TRUE(rectOpt.has_value());
  const auto rect = rectOpt.value();
  const auto screen = tiny_skia::intRectToScreen(rect);
  ASSERT_TRUE(screen.has_value());
  EXPECT_EQ(screen.value().x(), 10u);
  EXPECT_EQ(screen.value().y(), 20u);

  const auto direct = rect.toScreenIntRect();
  ASSERT_TRUE(direct.has_value());
  EXPECT_EQ(direct.value().x(), 10u);
  EXPECT_EQ(direct.value().y(), 20u);
}

TEST(GeomTest, RectFromLtrbRejectsInvalidBounds) {
  EXPECT_FALSE(tiny_skia::Rect::fromLtrb(2.0f, 1.0f, 1.0f, 2.0f).has_value());
  EXPECT_FALSE(tiny_skia::Rect::fromLtrb(1.0f, 2.0f, 1.0f, 1.0f).has_value());
}
