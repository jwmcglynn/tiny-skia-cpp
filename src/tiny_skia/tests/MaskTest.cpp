#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Mask.h"
#include "tiny_skia/Pixmap.h"

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

TEST(MaskTest, FromPixmapAlphaCopiesAlphaChannel) {
  const auto size = tiny_skia::IntSize::fromWh(2, 1);
  ASSERT_TRUE(size.has_value());
  const auto pixmap =
      tiny_skia::Pixmap::fromVec(std::vector<std::uint8_t>{10, 20, 30, 40, 50, 60, 70, 80},
                                 size.value());
  ASSERT_TRUE(pixmap.has_value());

  const auto mask = tiny_skia::Mask::fromPixmap(pixmap->asRef(), tiny_skia::MaskType::Alpha);
  ASSERT_EQ(mask.data().size(), 2u);
  EXPECT_EQ(mask.data()[0], 40u);
  EXPECT_EQ(mask.data()[1], 80u);
}

TEST(MaskTest, FromPixmapLuminanceUsesDemultiplyThenLumaTimesAlpha) {
  const auto size = tiny_skia::IntSize::fromWh(3, 1);
  ASSERT_TRUE(size.has_value());
  const auto pixmap = tiny_skia::Pixmap::fromVec(
      std::vector<std::uint8_t>{
          255, 0, 0, 255,
          64, 0, 0, 128,
          200, 100, 50, 0,
      },
      size.value());
  ASSERT_TRUE(pixmap.has_value());

  const auto mask = tiny_skia::Mask::fromPixmap(pixmap->asRef(), tiny_skia::MaskType::Luminance);
  ASSERT_EQ(mask.data().size(), 3u);
  EXPECT_EQ(mask.data()[0], 55u);
  EXPECT_EQ(mask.data()[1], 14u);
  EXPECT_EQ(mask.data()[2], 0u);
}

TEST(MaskTest, SubmaskComputesIntersectedViewAndOffset) {
  auto maskOpt = tiny_skia::Mask::fromSize(4, 3);
  ASSERT_TRUE(maskOpt.has_value());
  auto mask = std::move(maskOpt.value());
  auto bytes = mask.dataMut();
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<std::uint8_t>(i);
  }

  const auto rect = tiny_skia::IntRect::fromXYWH(1, 1, 2, 1);
  ASSERT_TRUE(rect.has_value());
  const auto sub = mask.submask(rect.value());
  ASSERT_TRUE(sub.has_value());
  EXPECT_EQ(sub->realWidth, 4u);
  EXPECT_EQ(sub->size.width(), 2u);
  EXPECT_EQ(sub->size.height(), 1u);
  EXPECT_EQ(sub->data[0], 5u);
  EXPECT_EQ(sub->data[1], 6u);
}

TEST(MaskTest, SubpixmapComputesIntersectedMutableView) {
  auto maskOpt = tiny_skia::Mask::fromSize(4, 3);
  ASSERT_TRUE(maskOpt.has_value());
  auto mask = std::move(maskOpt.value());
  auto bytes = mask.dataMut();
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    bytes[i] = static_cast<std::uint8_t>(i);
  }

  const auto rect = tiny_skia::IntRect::fromXYWH(3, 2, 3, 2);
  ASSERT_TRUE(rect.has_value());
  const auto sub = mask.subpixmap(rect.value());
  ASSERT_TRUE(sub.has_value());
  EXPECT_EQ(sub->realWidth, 4u);
  EXPECT_EQ(sub->size.width(), 1u);
  EXPECT_EQ(sub->size.height(), 1u);
  EXPECT_EQ(sub->data[0], 11u);

  sub->data[0] = 99u;
  EXPECT_EQ(mask.data()[11], 99u);
}
