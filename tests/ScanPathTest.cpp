#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/scan/Mod.h"
#include "tiny_skia/scan/Path.h"
#include "tiny_skia/scan/PathAa.h"
#include "tiny_skia/scan/Hairline.h"
#include "tiny_skia/scan/HairlineAa.h"

namespace {

struct ScanSpan {
  std::uint32_t x = 0;
  std::uint32_t y = 0;
  std::uint32_t width = 0;
};

class RecordingBlitter final : public tiny_skia::Blitter {
 public:
  void blitH(std::uint32_t x, std::uint32_t y, tiny_skia::LengthU32 width) override {
    spans_.push_back(ScanSpan{x, y, width});
  }

  void blitRect(const tiny_skia::ScreenIntRect& rect) override {
    rects_.push_back(rect);
  }

  [[nodiscard]] const std::vector<ScanSpan>& spans() const {
    return spans_;
  }

  [[nodiscard]] const std::vector<tiny_skia::ScreenIntRect>& rects() const {
    return rects_;
  }

 private:
  std::vector<ScanSpan> spans_;
  std::vector<tiny_skia::ScreenIntRect> rects_;
};

struct ScanPathAaSpan {
  std::uint32_t x = 0;
  std::uint32_t y = 0;
  std::uint32_t width = 0;
};

class RecordingAntialiasBlitter final : public tiny_skia::Blitter {
 public:
  void blitH(std::uint32_t x, std::uint32_t y, tiny_skia::LengthU32 width) override {
    spans_.push_back(ScanPathAaSpan{x, y, width});
  }

  void blitRect(const tiny_skia::ScreenIntRect& rect) override {
    for (std::uint32_t y = rect.y(); y < rect.y() + rect.height(); ++y) {
      spans_.push_back(ScanPathAaSpan{rect.x(), y, rect.width()});
    }
  }

  void blitV(std::uint32_t x,
             std::uint32_t y,
             tiny_skia::LengthU32 height,
             std::uint8_t /*alpha*/) override {
    spans_.push_back(ScanPathAaSpan{x, y, height});
    if (height > 1) {
      spans_.push_back(ScanPathAaSpan{x, y + height - 1, 1});
    }
  }

  void blitAntiH(std::uint32_t x,
                 std::uint32_t y,
                 std::span<std::uint8_t> alpha,
                 std::span<tiny_skia::AlphaRun> runs) override {
    antiSpans_.push_back(ScanPathAaSpan{x, y, antiWidth(runs)});
  }

  void blitAntiH2(std::uint32_t x,
                  std::uint32_t y,
                  std::uint8_t alpha0,
                  std::uint8_t alpha1) override {
    std::array<std::uint8_t, 2> alpha{alpha0, alpha1};
    std::array<tiny_skia::AlphaRun, 3> runs{
        static_cast<std::uint16_t>(1), static_cast<std::uint16_t>(1), std::nullopt};
    blitAntiH(x, y, std::span<std::uint8_t>{alpha}, std::span<tiny_skia::AlphaRun>{runs});
  }

  void blitAntiV2(std::uint32_t x,
                  std::uint32_t y,
                  std::uint8_t alpha0,
                  std::uint8_t alpha1) override {
    antiSpans_.push_back(ScanPathAaSpan{x, y, static_cast<std::uint32_t>(alpha0 != 0)});
    antiSpans_.push_back(ScanPathAaSpan{x, y + 1, static_cast<std::uint32_t>(alpha1 != 0)});
  }

  [[nodiscard]] const std::vector<ScanPathAaSpan>& spans() const {
    return spans_;
  }

  [[nodiscard]] const std::vector<ScanPathAaSpan>& antiSpans() const {
    return antiSpans_;
  }

 private:
  static std::uint32_t antiWidth(std::span<tiny_skia::AlphaRun> runs) {
    std::uint32_t width = 0;
    for (std::size_t i = 0; i < runs.size();) {
      if (!runs[i].has_value()) {
        break;
      }
      const auto run = runs[i].value();
      width += static_cast<std::uint32_t>(run);
      i += static_cast<std::size_t>(run);
    }
    return width;
  }

  std::vector<ScanPathAaSpan> spans_;
  std::vector<ScanPathAaSpan> antiSpans_;
};

}  // namespace

TEST(ScanPathTest, FillPathWithNoEdgesProducesNoSpans) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  tiny_skia::Path path;
  RecordingBlitter blitter;

  tiny_skia::scan::fillPath(path, tiny_skia::FillRule::Winding, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
}

TEST(ScanPathTest, FillPathFilledRectangleProducesExpectedSpans) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 8.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({2.0f, 8.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::fillPath(path, tiny_skia::FillRule::EvenOdd, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(6));
  for (std::uint32_t i = 0; i < 6; ++i) {
    EXPECT_EQ(blitter.spans()[i].x, 2u);
    EXPECT_EQ(blitter.spans()[i].y, 2u + i);
    EXPECT_EQ(blitter.spans()[i].width, 6u);
  }
}

TEST(ScanPathTest, FillPathOutsideClipCullsWithoutSpans) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({12.0f, 1.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 1.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 9.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({12.0f, 9.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::fillPath(path, tiny_skia::FillRule::Winding, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
}

TEST(ScanPathTest, FillPathAaFallsBackToAntialiasBlits) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 8.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({2.0f, 8.0f});

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::path_aa::fillPath(path, tiny_skia::FillRule::Winding, clip, blitter);

  EXPECT_GT(blitter.antiSpans().size(), 0u);
  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
}

TEST(ScanPathTest, FillPathAaClipOverflowAvoidsBlitting) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 40000, 2).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 0.5f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({6.0f, 0.5f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({6.0f, 1.5f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({2.0f, 1.5f});

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::path_aa::fillPath(path, tiny_skia::FillRule::Winding, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
  EXPECT_THAT(blitter.antiSpans(), ::testing::IsEmpty());
}

TEST(ScanPathTest, FillRectRoundsOutAndClipsToClipRect) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  const auto rect = tiny_skia::Rect::fromLtrb(2.2f, 2.7f, 6.3f, 6.1f).value();

  RecordingBlitter blitter;
  tiny_skia::scan::fillRect(rect, clip, blitter);

  EXPECT_THAT(blitter.rects(), ::testing::SizeIs(1));
  EXPECT_EQ(blitter.rects()[0].x(), 2u);
  EXPECT_EQ(blitter.rects()[0].y(), 2u);
  EXPECT_EQ(blitter.rects()[0].width(), 5u);
  EXPECT_EQ(blitter.rects()[0].height(), 5u);
}

TEST(ScanPathTest, FillRectClipsOutOfRange) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  const auto rect = tiny_skia::Rect::fromLtrb(18.2f, 18.8f, 25.1f, 27.9f).value();

  RecordingBlitter blitter;
  tiny_skia::scan::fillRect(rect, clip, blitter);

  EXPECT_THAT(blitter.rects(), ::testing::SizeIs(1));
  EXPECT_EQ(blitter.rects()[0].x(), 18u);
  EXPECT_EQ(blitter.rects()[0].y(), 18u);
  EXPECT_EQ(blitter.rects()[0].width(), 2u);
  EXPECT_EQ(blitter.rects()[0].height(), 2u);
}

TEST(ScanPathTest, FillRectClippedAwayWhenOutsideClip) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  const auto rect = tiny_skia::Rect::fromLtrb(12.0f, 12.0f, 20.0f, 20.0f).value();

  RecordingBlitter blitter;
  tiny_skia::scan::fillRect(rect, clip, blitter);

  EXPECT_THAT(blitter.rects(), ::testing::IsEmpty());
}

TEST(ScanPathTest, FillRectAaUsesAntialiasingForFractionalCoordinates) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  const auto rect = tiny_skia::Rect::fromLtrb(2.2f, 2.2f, 5.8f, 5.8f).value();

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::fillRectAa(rect, clip, blitter);

  EXPECT_THAT(blitter.antiSpans(), ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(blitter.spans(), ::testing::Not(::testing::IsEmpty()));
}

TEST(ScanPathTest, FillRectAaIntegerRectUsesWholeRectBlit) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  const auto rect = tiny_skia::Rect::fromLtrb(2.0f, 2.0f, 6.0f, 6.0f).value();

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::fillRectAa(rect, clip, blitter);

  EXPECT_THAT(blitter.antiSpans(), ::testing::IsEmpty());
  EXPECT_THAT(blitter.spans(), ::testing::SizeIs(4));
  for (std::uint32_t i = 0; i < 4; ++i) {
    EXPECT_EQ(blitter.spans()[i].x, 2u);
    EXPECT_EQ(blitter.spans()[i].y, 2u + i);
    EXPECT_EQ(blitter.spans()[i].width, 4u);
  }
}

TEST(ScanPathTest, FillRectAaClipsOutsideClipRect) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  const auto rect = tiny_skia::Rect::fromLtrb(-2.5f, 8.5f, 12.7f, 14.2f).value();

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::fillRectAa(rect, clip, blitter);

  EXPECT_THAT(blitter.antiSpans(), ::testing::Not(::testing::IsEmpty()));
  EXPECT_THAT(blitter.spans(), ::testing::Not(::testing::IsEmpty()));
}

TEST(ScanPathTest, StrokePathButtHorizontalLine) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(6u));
  for (std::size_t i = 0; i < blitter.spans().size(); ++i) {
    EXPECT_EQ(blitter.spans()[i].x, static_cast<std::uint32_t>(2u + i));
    EXPECT_EQ(blitter.spans()[i].y, 2u);
    EXPECT_EQ(blitter.spans()[i].width, 1u);
  }
}

TEST(ScanPathTest, StrokePathButtVerticalLine) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({5.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({5.0f, 8.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(6u));
  for (std::size_t i = 0; i < blitter.spans().size(); ++i) {
    EXPECT_EQ(blitter.spans()[i].x, 5u);
    EXPECT_EQ(blitter.spans()[i].y, 2u + i);
    EXPECT_EQ(blitter.spans()[i].width, 1u);
  }
}

TEST(ScanPathTest, StrokePathRoundCapExtendsSubpixelLine) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.6f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({4.6f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Round, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(3u));
  EXPECT_EQ(blitter.spans()[0].x, 2u);
  EXPECT_EQ(blitter.spans()[0].y, 2u);
  EXPECT_EQ(blitter.spans()[0].width, 1u);
  EXPECT_EQ(blitter.spans()[1].x, 3u);
  EXPECT_EQ(blitter.spans()[1].y, 2u);
  EXPECT_EQ(blitter.spans()[1].width, 1u);
  EXPECT_EQ(blitter.spans()[2].x, 4u);
  EXPECT_EQ(blitter.spans()[2].y, 2u);
  EXPECT_EQ(blitter.spans()[2].width, 1u);
}

TEST(ScanPathTest, StrokePathQuadAsLineButt) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({5.0f, 2.0f});
  path.addPoint({8.0f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(3u));
  EXPECT_EQ(blitter.spans()[0].x, 2u);
  EXPECT_EQ(blitter.spans()[0].y, 2u);
  EXPECT_EQ(blitter.spans()[0].width, 1u);
  EXPECT_EQ(blitter.spans()[1].x, 3u);
  EXPECT_EQ(blitter.spans()[1].y, 2u);
  EXPECT_EQ(blitter.spans()[1].width, 1u);
  EXPECT_EQ(blitter.spans()[2].x, 4u);
  EXPECT_EQ(blitter.spans()[2].y, 2u);
  EXPECT_EQ(blitter.spans()[2].width, 1u);
}

TEST(ScanPathTest, StrokePathCubicAsLineButt) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Cubic);
  path.addPoint({4.0f, 2.0f});
  path.addPoint({6.0f, 2.0f});
  path.addPoint({8.0f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_THAT(blitter.spans(), ::testing::SizeIs(2u));
  EXPECT_EQ(blitter.spans()[0].x, 2u);
  EXPECT_EQ(blitter.spans()[0].y, 2u);
  EXPECT_EQ(blitter.spans()[0].width, 1u);
  EXPECT_EQ(blitter.spans()[1].x, 3u);
  EXPECT_EQ(blitter.spans()[1].y, 2u);
  EXPECT_EQ(blitter.spans()[1].width, 1u);
}

TEST(ScanPathTest, StrokePathQuadCurved) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 30, 30).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({10.0f, 14.0f});
  path.addPoint({18.0f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_GT(blitter.spans().size(), 0u);
  EXPECT_GE(blitter.spans().front().x, 2u);
  EXPECT_LE(blitter.spans().front().y, 14u);
  EXPECT_GE(blitter.spans().back().y, 2u);
}

TEST(ScanPathTest, StrokePathCubicCurved) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 30, 30).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Cubic);
  path.addPoint({6.0f, 18.0f});
  path.addPoint({22.0f, 18.0f});
  path.addPoint({26.0f, 2.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  ASSERT_GT(blitter.spans().size(), 0u);
  EXPECT_LE(blitter.spans().front().y, 18u);
  EXPECT_GE(blitter.spans().back().y, 2u);
  for (const auto& span : blitter.spans()) {
    EXPECT_EQ(span.width, 1u);
    EXPECT_GE(span.y, 0u);
  }
}

TEST(ScanPathTest, StrokePathAaButtHorizontalLine) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 20, 20).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({8.0f, 2.0f});

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::hairline_aa::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
  EXPECT_GT(blitter.antiSpans().size(), 0u);
  EXPECT_LT(blitter.antiSpans()[0].y, 255u);
}

TEST(ScanPathTest, StrokePathAaQuadLine) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 24, 24).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({8.0f, 14.0f});
  path.addPoint({14.0f, 2.0f});

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::hairline_aa::strokePath(path, tiny_skia::LineCap::Round, clip, blitter);

  EXPECT_TRUE(!blitter.antiSpans().empty());
  for (const auto& span : blitter.antiSpans()) {
    EXPECT_GT(span.width, 0u);
  }
}

TEST(ScanPathTest, StrokePathAaLineClippedOutside) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({12.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 2.0f});

  RecordingAntialiasBlitter blitter;
  tiny_skia::scan::hairline_aa::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
  EXPECT_THAT(blitter.antiSpans(), ::testing::IsEmpty());
}

TEST(ScanPathTest, StrokePathOutsideClipCullsWithoutSpans) {
  const auto clip = tiny_skia::ScreenIntRect::fromXYWH(0, 0, 10, 10).value();
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({12.0f, 1.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 1.0f});

  RecordingBlitter blitter;
  tiny_skia::scan::strokePath(path, tiny_skia::LineCap::Butt, clip, blitter);

  EXPECT_THAT(blitter.spans(), ::testing::IsEmpty());
}
