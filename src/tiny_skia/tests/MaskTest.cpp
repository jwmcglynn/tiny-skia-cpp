#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Mask.h"

TEST(MaskTest, FromSizeRejectsZeroDimensions) {
  EXPECT_FALSE(tiny_skia::Mask::fromSize(0, 1).has_value());
  EXPECT_FALSE(tiny_skia::Mask::fromSize(1, 0).has_value());
}

TEST(MaskTest, FromSizeInitializesZeroedBufferAndDimensions) {
  const auto maskOpt = tiny_skia::Mask::fromSize(3, 2);
  ASSERT_TRUE(maskOpt.has_value());
  const auto mask = maskOpt.value();

  EXPECT_EQ(mask.width(), 3u);
  EXPECT_EQ(mask.height(), 2u);
  EXPECT_EQ(mask.data().size(), 6u);
  for (const auto value : mask.data()) {
    EXPECT_EQ(value, 0u);
  }
}

TEST(MaskTest, FromVecRequiresExactSize) {
  const auto size = tiny_skia::IntSize::fromWh(2, 2);
  ASSERT_TRUE(size.has_value());

  EXPECT_FALSE(tiny_skia::Mask::fromVec(std::vector<std::uint8_t>{1, 2, 3}, size.value()).has_value());
  EXPECT_TRUE(
      tiny_skia::Mask::fromVec(std::vector<std::uint8_t>{1, 2, 3, 4}, size.value()).has_value());
}

TEST(MaskTest, DataMutAndTakeExposeOwnedBuffer) {
  auto maskOpt = tiny_skia::Mask::fromSize(2, 2);
  ASSERT_TRUE(maskOpt.has_value());
  auto mask = std::move(maskOpt.value());

  auto writable = mask.dataMut();
  writable[0] = 7;
  writable[3] = 11;
  EXPECT_EQ(mask.data()[0], 7u);
  EXPECT_EQ(mask.data()[3], 11u);

  const auto taken = mask.take();
  ASSERT_EQ(taken.size(), 4u);
  EXPECT_EQ(taken[0], 7u);
  EXPECT_EQ(taken[3], 11u);
  EXPECT_TRUE(mask.data().empty());
  EXPECT_EQ(mask.width(), 0u);
  EXPECT_EQ(mask.height(), 0u);
}
