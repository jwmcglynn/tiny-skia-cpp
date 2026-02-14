#include <cstdint>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/Geom.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/scan/Path.h"
#include "tiny_skia/scan/PathAa.h"

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

  [[nodiscard]] const std::vector<ScanSpan>& spans() const {
    return spans_;
  }

 private:
  std::vector<ScanSpan> spans_;
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

  void blitAntiH(std::uint32_t x,
                 std::uint32_t y,
                 std::span<std::uint8_t> alpha,
                 std::span<tiny_skia::AlphaRun> runs) override {
    antiSpans_.push_back(ScanPathAaSpan{x, y,
                                        static_cast<std::uint32_t>(alpha.size() + runs.size())});
  }

  [[nodiscard]] const std::vector<ScanPathAaSpan>& spans() const {
    return spans_;
  }

  [[nodiscard]] const std::vector<ScanPathAaSpan>& antiSpans() const {
    return antiSpans_;
  }

 private:
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
