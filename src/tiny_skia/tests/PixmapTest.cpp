#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

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
