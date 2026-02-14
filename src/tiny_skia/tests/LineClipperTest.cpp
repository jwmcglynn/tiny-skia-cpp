#include <array>
#include <span>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"
#include "tiny_skia/LineClipper.h"

namespace {

void ExpectPoints(std::span<const tiny_skia::Point> actual,
                 std::initializer_list<tiny_skia::Point> expected,
                 const std::string& context) {
  EXPECT_THAT(actual.size(), testing::Eq(expected.size())) << context;
  int index = 0;
  for (const auto& point : expected) {
    ASSERT_LT(index, static_cast<int>(actual.size())) << context;
    EXPECT_FLOAT_EQ(actual[index].x, point.x) << "index " << index << " x: " << context;
    EXPECT_FLOAT_EQ(actual[index].y, point.y) << "index " << index << " y: " << context;
    ++index;
  }
}

}  // namespace

TEST(LineClipperTest, ClipRejectsFullyAboveAndBelow) {
  const auto clip = tiny_skia::Rect::fromLtrb(0.0f, 0.0f, 10.0f, 10.0f).value();
  std::array<tiny_skia::Point, 2> src{{{0.0f, -5.0f}, {5.0f, -1.0f}}};
  std::array<tiny_skia::Point, tiny_skia::line_clipper::kLineClipperMaxPoints> out{};

  auto noClip = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(src),
                                              clip,
                                              false,
                                              std::span<tiny_skia::Point, 4>(out));
  EXPECT_TRUE(noClip.empty());

  std::array<tiny_skia::Point, 2> belowSrc{{{0.0f, 12.0f}, {5.0f, 20.0f}}};
  auto noClipBelow = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(belowSrc),
                                                   clip,
                                                   false,
                                                   std::span<tiny_skia::Point, 4>(out));
  EXPECT_TRUE(noClipBelow.empty());
}

TEST(LineClipperTest, ClipClampsHorizontallyInsideBounds) {
  const auto clip = tiny_skia::Rect::fromLtrb(1.0f, 0.0f, 9.0f, 10.0f).value();
  std::array<tiny_skia::Point, 2> src{{{-5.0f, 3.0f}, {5.0f, 3.0f}}};
  std::array<tiny_skia::Point, tiny_skia::line_clipper::kLineClipperMaxPoints> out{};

  auto result = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(src),
                                             clip,
                                             false,
                                             std::span<tiny_skia::Point, 4>(out));
  ExpectPoints(result,
              {tiny_skia::Point{1.0f, 3.0f}, tiny_skia::Point{1.0f, 3.0f}, tiny_skia::Point{5.0f, 3.0f}},
              "horizontal segment clipped from left");
}

TEST(LineClipperTest, ClipClampsBothSidesOnSkewLine) {
  const auto clip = tiny_skia::Rect::fromLtrb(0.0f, 0.0f, 10.0f, 10.0f).value();
  std::array<tiny_skia::Point, 2> src{{{-5.0f, 2.0f}, {15.0f, 8.0f}}};
  std::array<tiny_skia::Point, tiny_skia::line_clipper::kLineClipperMaxPoints> out{};

  auto result = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(src),
                                             clip,
                                             false,
                                             std::span<tiny_skia::Point, 4>(out));
  ExpectPoints(result,
              {tiny_skia::Point{0.0f, 2.0f},
               tiny_skia::Point{0.0f, 3.5f},
               tiny_skia::Point{10.0f, 6.5f},
               tiny_skia::Point{10.0f, 8.0f}},
              "diagonal segment clipped across both edges");
}

TEST(LineClipperTest, ClipCanCullToRightAndPreserveWhenFalse) {
  const auto clip = tiny_skia::Rect::fromLtrb(1.0f, 0.0f, 10.0f, 10.0f).value();
  std::array<tiny_skia::Point, 2> src{{{12.0f, 2.0f}, {20.0f, 4.0f}}};
  std::array<tiny_skia::Point, tiny_skia::line_clipper::kLineClipperMaxPoints> out{};

  auto culled = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(src),
                                             clip,
                                             true,
                                             std::span<tiny_skia::Point, 4>(out));
  EXPECT_TRUE(culled.empty());

  auto preserved = tiny_skia::line_clipper::clip(std::span<const tiny_skia::Point, 2>(src),
                                                clip,
                                                false,
                                                std::span<tiny_skia::Point, 4>(out));
  ExpectPoints(preserved,
              {tiny_skia::Point{10.0f, 2.0f}, tiny_skia::Point{10.0f, 4.0f}},
              "horizontal clip preserves right edge when culling disabled");
}

TEST(LineClipperTest, IntersectClipsAndReturnsTrueForPartiallyOverlapping) {
  const auto clip = tiny_skia::Rect::fromLtrb(1.0f, 1.0f, 9.0f, 9.0f).value();
  std::array<tiny_skia::Point, 2> src{{{-5.0f, -5.0f}, {15.0f, 15.0f}}};
  std::array<tiny_skia::Point, 2> dst{};

  auto intersects = tiny_skia::line_clipper::intersect(std::span<const tiny_skia::Point, 2>(src),
                                                      clip,
                                                      std::span<tiny_skia::Point, 2>(dst));
  EXPECT_TRUE(intersects);
  EXPECT_FLOAT_EQ(dst[0].x, 1.0f);
  EXPECT_FLOAT_EQ(dst[0].y, 1.0f);
  EXPECT_FLOAT_EQ(dst[1].x, 9.0f);
  EXPECT_FLOAT_EQ(dst[1].y, 9.0f);
}

TEST(LineClipperTest, IntersectRejectsDisjointSegment) {
  const auto clip = tiny_skia::Rect::fromLtrb(1.0f, 1.0f, 9.0f, 9.0f).value();
  std::array<tiny_skia::Point, 2> src{{{-5.0f, 3.0f}, {-1.0f, 7.0f}}};
  std::array<tiny_skia::Point, 2> dst{};

  auto intersects = tiny_skia::line_clipper::intersect(std::span<const tiny_skia::Point, 2>(src),
                                                      clip,
                                                      std::span<tiny_skia::Point, 2>(dst));
  EXPECT_FALSE(intersects);
}
