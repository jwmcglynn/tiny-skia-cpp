#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "tiny_skia/Color.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Pixmap.h"

TEST(PixmapTest, FromSizeRejectsZeroAndTooWideInputs) {
  EXPECT_FALSE(tiny_skia::Pixmap::fromSize(0, 1).has_value());
  EXPECT_FALSE(tiny_skia::Pixmap::fromSize(1, 0).has_value());

  const auto tooWide = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max() / 4) + 1u;
  EXPECT_FALSE(tiny_skia::Pixmap::fromSize(tooWide, 1).has_value());
}

TEST(PixmapTest, FromSizeAllocatesZeroedRgbaBuffer) {
  const auto pixmapOpt = tiny_skia::Pixmap::fromSize(3, 2);
  ASSERT_TRUE(pixmapOpt.has_value());
  const auto pixmap = pixmapOpt.value();

  EXPECT_EQ(pixmap.width(), 3u);
  EXPECT_EQ(pixmap.height(), 2u);
  ASSERT_EQ(pixmap.data().size(), 24u);
  for (const auto v : pixmap.data()) {
    EXPECT_EQ(v, 0u);
  }
}

TEST(PixmapTest, FromVecValidatesExactByteLength) {
  const auto size = tiny_skia::IntSize::fromWh(2, 2);
  ASSERT_TRUE(size.has_value());

  EXPECT_FALSE(tiny_skia::Pixmap::fromVec(std::vector<std::uint8_t>(15, 0), size.value()).has_value());
  EXPECT_TRUE(tiny_skia::Pixmap::fromVec(std::vector<std::uint8_t>(16, 0), size.value()).has_value());
}

TEST(PixmapTest, PixmapRefFromBytesAndPixelAccessMatchRgbaPacking) {
  const std::vector<std::uint8_t> bytes{
      1, 2, 3, 4,
      5, 6, 7, 8,
  };

  auto refOpt = tiny_skia::PixmapRef::fromBytes(bytes, 2, 1);
  ASSERT_TRUE(refOpt.has_value());
  const auto ref = refOpt.value();

  EXPECT_EQ(ref.width(), 2u);
  EXPECT_EQ(ref.height(), 1u);
  ASSERT_EQ(ref.pixels().size(), 2u);
  EXPECT_EQ(ref.pixels()[0].red(), 1u);
  EXPECT_EQ(ref.pixels()[0].green(), 2u);
  EXPECT_EQ(ref.pixels()[0].blue(), 3u);
  EXPECT_EQ(ref.pixels()[0].alpha(), 4u);
  EXPECT_EQ(ref.pixels()[1].red(), 5u);
  EXPECT_EQ(ref.pixels()[1].green(), 6u);
  EXPECT_EQ(ref.pixels()[1].blue(), 7u);
  EXPECT_EQ(ref.pixels()[1].alpha(), 8u);

  const auto p = ref.pixel(1, 0);
  ASSERT_TRUE(p.has_value());
  EXPECT_EQ(p->red(), 5u);
  EXPECT_FALSE(ref.pixel(2, 0).has_value());
}

TEST(PixmapTest, DataMutPixelsMutAndTakeExposeOwnedStorage) {
  auto pixmapOpt = tiny_skia::Pixmap::fromSize(1, 1);
  ASSERT_TRUE(pixmapOpt.has_value());
  auto pixmap = std::move(pixmapOpt.value());

  auto mutableBytes = pixmap.dataMut();
  mutableBytes[0] = 9;
  mutableBytes[1] = 10;
  mutableBytes[2] = 11;
  mutableBytes[3] = 12;

  ASSERT_EQ(pixmap.pixelsMut().size(), 1u);
  EXPECT_EQ(pixmap.pixelsMut()[0].red(), 9u);
  EXPECT_EQ(pixmap.pixelsMut()[0].green(), 10u);
  EXPECT_EQ(pixmap.pixelsMut()[0].blue(), 11u);
  EXPECT_EQ(pixmap.pixelsMut()[0].alpha(), 12u);

  const auto taken = pixmap.take();
  ASSERT_EQ(taken.size(), 4u);
  EXPECT_EQ(taken[0], 9u);
  EXPECT_EQ(taken[3], 12u);
  EXPECT_TRUE(pixmap.data().empty());
  EXPECT_EQ(pixmap.width(), 0u);
  EXPECT_EQ(pixmap.height(), 0u);
}

TEST(PixmapTest, FillAndTakeDemultipliedMatchColorConversion) {
  auto pixmapOpt = tiny_skia::Pixmap::fromSize(2, 1);
  ASSERT_TRUE(pixmapOpt.has_value());
  auto pixmap = std::move(pixmapOpt.value());

  const auto color = tiny_skia::Color::fromRgba8(255, 0, 0, 128);
  pixmap.fill(color);
  ASSERT_EQ(pixmap.pixels().size(), 2u);
  EXPECT_EQ(pixmap.pixels()[0].red(), 128u);
  EXPECT_EQ(pixmap.pixels()[0].green(), 0u);
  EXPECT_EQ(pixmap.pixels()[0].blue(), 0u);
  EXPECT_EQ(pixmap.pixels()[0].alpha(), 128u);

  const auto demul = pixmap.takeDemultiplied();
  ASSERT_EQ(demul.size(), 8u);
  EXPECT_EQ(demul[0], 255u);
  EXPECT_EQ(demul[1], 0u);
  EXPECT_EQ(demul[2], 0u);
  EXPECT_EQ(demul[3], 128u);
}

TEST(PixmapTest, CloneRectCopiesContainedRegion) {
  const auto size = tiny_skia::IntSize::fromWh(3, 2);
  ASSERT_TRUE(size.has_value());
  auto pixmap =
      tiny_skia::Pixmap::fromVec(std::vector<std::uint8_t>{
                                     1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                     13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                 },
                                 size.value());
  ASSERT_TRUE(pixmap.has_value());
  const auto rect = tiny_skia::IntRect::fromXYWH(1, 0, 2, 2);
  ASSERT_TRUE(rect.has_value());

  const auto cloned = pixmap->cloneRect(rect.value());
  ASSERT_TRUE(cloned.has_value());
  EXPECT_EQ(cloned->width(), 2u);
  EXPECT_EQ(cloned->height(), 2u);
  ASSERT_EQ(cloned->data().size(), 16u);
  EXPECT_EQ(cloned->data()[0], 5u);
  EXPECT_EQ(cloned->data()[1], 6u);
  EXPECT_EQ(cloned->data()[2], 7u);
  EXPECT_EQ(cloned->data()[3], 8u);
  EXPECT_EQ(cloned->data()[12], 21u);
  EXPECT_EQ(cloned->data()[15], 24u);
}

TEST(PixmapTest, PixmapMutFromBytesAndSubpixmapProvideMutableSubview) {
  std::vector<std::uint8_t> bytes{
      1, 2, 3, 4, 5, 6, 7, 8,
      9, 10, 11, 12, 13, 14, 15, 16,
  };
  auto mut = tiny_skia::PixmapMut::fromBytes(bytes, 2, 2);
  ASSERT_TRUE(mut.has_value());
  ASSERT_EQ(mut->pixelsMut().size(), 4u);
  EXPECT_EQ(mut->pixelsMut()[0].alpha(), 4u);

  const auto rect = tiny_skia::IntRect::fromXYWH(1, 1, 1, 1);
  ASSERT_TRUE(rect.has_value());
  const auto sub = mut->subpixmap(rect.value());
  ASSERT_TRUE(sub.has_value());
  EXPECT_EQ(sub->size.width(), 1u);
  EXPECT_EQ(sub->size.height(), 1u);
  EXPECT_EQ(sub->real_width, 2u);
  EXPECT_EQ(sub->data[0], 13u);
  sub->data[0] = 99u;
  EXPECT_EQ(bytes[12], 99u);
}
