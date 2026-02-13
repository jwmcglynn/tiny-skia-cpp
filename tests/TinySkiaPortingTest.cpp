#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/AlphaRuns.h"
#include "tiny_skia/Color.h"
#include "tiny_skia/FixedPoint.h"
#include "tiny_skia/Math.h"

namespace {

using ::testing::ElementsAre;

constexpr float kFloatTolerance = 0.0001f;
constexpr std::string_view kOpaque = "opaque";

void expectColorU8Is(const tiny_skia::ColorU8& actual,
                     std::uint8_t red,
                     std::uint8_t green,
                     std::uint8_t blue,
                     std::uint8_t alpha) {
  EXPECT_EQ(actual.red(), red) << "r";
  EXPECT_EQ(actual.green(), green) << "g";
  EXPECT_EQ(actual.blue(), blue) << "b";
  EXPECT_EQ(actual.alpha(), alpha) << "a";
}

void expectColorU8Is(const tiny_skia::PremultipliedColorU8& actual,
                     std::uint8_t red,
                     std::uint8_t green,
                     std::uint8_t blue,
                     std::uint8_t alpha) {
  expectColorU8Is(tiny_skia::ColorU8(actual.red(), actual.green(), actual.blue(), actual.alpha()),
                  red,
                  green,
                  blue,
                  alpha);
}

void expectColorFloatIs(const tiny_skia::Color& actual,
                       float red,
                       float green,
                       float blue,
                       float alpha) {
  EXPECT_NEAR(actual.red(), red, kFloatTolerance) << "r";
  EXPECT_NEAR(actual.green(), green, kFloatTolerance) << "g";
  EXPECT_NEAR(actual.blue(), blue, kFloatTolerance) << "b";
  EXPECT_NEAR(actual.alpha(), alpha, kFloatTolerance) << "a";
}

}  // namespace

TEST(MathTest, BoundClampsToRange) {
  EXPECT_EQ(tiny_skia::bound(0u, 3u, 10u), 3u);
  EXPECT_EQ(tiny_skia::bound(5u, 3u, 10u), 5u);
  EXPECT_EQ(tiny_skia::bound(0u, 13u, 10u), 10u);
}

TEST(MathTest, LeftShiftAndApproxPowf) {
  EXPECT_EQ(tiny_skia::leftShift(3, 4), 48);
  EXPECT_EQ(tiny_skia::leftShift64(-1, 1),
            static_cast<std::int64_t>(0xFFFFFFFFFFFFFFFEULL));

  EXPECT_FLOAT_EQ(tiny_skia::approxPowf(0.0f, 3.0f), 0.0f);
  EXPECT_FLOAT_EQ(tiny_skia::approxPowf(1.0f, 3.0f), 1.0f);
  EXPECT_NEAR(tiny_skia::approxPowf(2.0f, 1.0f), 2.0f, kFloatTolerance);
  EXPECT_NEAR(tiny_skia::approxPowf(4.0f, 1.0f), 4.0f, kFloatTolerance);
}

TEST(FixedPointFdot6Test, ConversionsAndRounding) {
  EXPECT_EQ(tiny_skia::fdot6::fromI32(3), 192);
  EXPECT_EQ(tiny_skia::fdot6::fromF32(1.5f), 96);
  EXPECT_EQ(tiny_skia::fdot6::floor(65), 1);
  EXPECT_EQ(tiny_skia::fdot6::ceil(65), 2);
  EXPECT_EQ(tiny_skia::fdot6::round(31), 0);
  EXPECT_EQ(tiny_skia::fdot6::toFdot16(64), 65536);
  EXPECT_EQ(tiny_skia::fdot6::div(64, 32), 131072);
  EXPECT_EQ(tiny_skia::fdot6::smallScale(255, 64), 255);

  struct CanConvertCase {
    std::string_view label;
    std::int32_t value;
    bool expected;
  };
  for (const auto& tc : {
           CanConvertCase{"safe bound", 30000, true},
           CanConvertCase{"overflow bound", std::numeric_limits<std::int32_t>::min(), false},
       }) {
    SCOPED_TRACE(tc.label);
    EXPECT_EQ(tiny_skia::fdot6::canConvertToFdot16(tc.value), tc.expected);
  }
}

TEST(Fdot8And16Test, ConversionsAndArithmetic) {
  EXPECT_EQ(tiny_skia::fdot8::fromFdot16(0xFF00), 0x0FF);

  EXPECT_EQ(tiny_skia::fdot16::one, 65536);
  EXPECT_EQ(tiny_skia::fdot16::fromF32(1.0f), 65536);
  EXPECT_EQ(tiny_skia::fdot16::floorToI32(65535), 0);
  EXPECT_EQ(tiny_skia::fdot16::ceilToI32(65535), 1);
  EXPECT_EQ(tiny_skia::fdot16::roundToI32(98304), 2);
  EXPECT_EQ(tiny_skia::fdot16::mul(65536, 65536), 65536);
  EXPECT_EQ(tiny_skia::fdot16::divide(65536, 65536), 65536);
  EXPECT_EQ(tiny_skia::fdot16::fastDiv(64, 32), 131072);
}

TEST(ColorTest, ColorUPremultiplyPreservesAlphaAndClamp) {
  const tiny_skia::ColorU8 source = tiny_skia::ColorU8::fromRgba(10, 20, 30, 40);
  const auto got = source.premultiply();
  const tiny_skia::PremultipliedColorU8 expected =
      tiny_skia::PremultipliedColorU8::fromRgbaUnchecked(2, 3, 5, 40);
  EXPECT_EQ(got, expected);

  const auto noOp = tiny_skia::ColorU8::fromRgba(10, 20, 30, tiny_skia::kAlphaU8Opaque);
  EXPECT_EQ(noOp.premultiply(),
            tiny_skia::PremultipliedColorU8::fromRgbaUnchecked(10, 20, 30, 255));

  const auto validated = tiny_skia::PremultipliedColorU8::fromRgba(2, 3, 5, 40);
  ASSERT_TRUE(validated.has_value()) << "valid premultiplied values must be accepted";
}

TEST(ColorTest, ColorUPremultiplyU8UsesFixedPointMultiply) {
  struct PremulCase {
    std::string_view label;
    std::uint8_t color;
    std::uint8_t alpha;
    std::uint8_t expected;
  };

  for (const auto& tc : {
           PremulCase{kOpaque, 255, 255, 255},
           PremulCase{"half", 2, 4, 0},
           PremulCase{"round", 10, 40, 2},
           PremulCase{"zero", 0, 255, 0},
       }) {
    SCOPED_TRACE(tc.label);
    EXPECT_EQ(tiny_skia::premultiplyU8(tc.color, tc.alpha), tc.expected);
  }
}

TEST(ColorTest, PremultipliedColorDemultiply) {
  struct DemulCase {
    std::string_view label;
    tiny_skia::PremultipliedColorU8 in;
    tiny_skia::ColorU8 expected;
  };

  for (const auto& tc : {
           DemulCase{"partial alpha",
                     tiny_skia::PremultipliedColorU8::fromRgbaUnchecked(2, 3, 5, 40),
                     tiny_skia::ColorU8::fromRgba(13, 19, 32, 40)},
           DemulCase{"opaque",
                     tiny_skia::PremultipliedColorU8::fromRgbaUnchecked(10, 20, 30, 255),
                     tiny_skia::ColorU8::fromRgba(10, 20, 30, 255)},
           DemulCase{"non-trivial",
                     tiny_skia::PremultipliedColorU8::fromRgbaUnchecked(153, 99, 54, 180),
                     tiny_skia::ColorU8::fromRgba(217, 140, 77, 180)},
       }) {
    SCOPED_TRACE(tc.label);
    EXPECT_EQ(tc.in.demultiply(), tc.expected);
  }
}

TEST(ColorTest, PremultipliedColorOptionValidation) {
  EXPECT_FALSE(tiny_skia::PremultipliedColorU8::fromRgba(255, 0, 0, 128).has_value())
      << "channel values above alpha must be invalid";
  EXPECT_TRUE(tiny_skia::PremultipliedColorU8::fromRgba(0, 0, 0, 0).has_value())
      << "all-zero must be valid";
}

TEST(ColorTest, ColorFromRgbaAndOpacity) {
  const tiny_skia::Color color = tiny_skia::Color::fromRgba8(255, 128, 64, 255);
  expectColorFloatIs(color, 1.0f, 128.0f / 255.0f, 64.0f / 255.0f, 1.0f);
  EXPECT_TRUE(color.isOpaque());

  const auto semiColor = tiny_skia::Color::fromRgba(1.0f, 0.5f, 0.25f, 1.0f);
  ASSERT_TRUE(semiColor.has_value());
  auto semi = *semiColor;
  semi.applyOpacity(0.5f);
  EXPECT_NEAR(semi.alpha(), 0.5f, kFloatTolerance);

  EXPECT_FALSE(tiny_skia::Color::fromRgba(1.1f, -0.1f, 0.0f, 0.5f).has_value())
      << "out of range color component is rejected";
}

TEST(ColorTest, ColorConversionToU8AndPremultiplyRoundTrip) {
  const auto colorOpt = tiny_skia::Color::fromRgba(1.0f, 0.5f, 0.0f, 0.5f);
  ASSERT_TRUE(colorOpt.has_value());
  const auto color = *colorOpt;
  const auto u8 = color.toColorU8();
  expectColorU8Is(u8, 255, 128, 0, 128);

  const auto pre = color.premultiply();
  const auto premulU8 = pre.toColorU8();
  expectColorU8Is(premulU8, 128, 64, 0, 128);
  EXPECT_EQ(premulU8.alpha(), 128);

  const auto unmul = pre.demultiply();
  expectColorFloatIs(unmul, 1.0f, 0.5f, 0.0f, 0.5f);
}

TEST(ColorTest, ColorSpaceTransforms) {
  using tiny_skia::ColorSpace;
  EXPECT_FALSE(tiny_skia::expandStage(ColorSpace::Linear).has_value());
  EXPECT_FALSE(tiny_skia::expandDestStage(ColorSpace::Linear).has_value());
  EXPECT_FALSE(tiny_skia::compressStage(ColorSpace::Linear).has_value());

  EXPECT_EQ(tiny_skia::expandStage(ColorSpace::Gamma2).value(),
            tiny_skia::pipeline::Stage::GammaExpand2);
  EXPECT_EQ(tiny_skia::expandDestStage(ColorSpace::SimpleSRGB).value(),
            tiny_skia::pipeline::Stage::GammaExpandDestination22);
  EXPECT_EQ(tiny_skia::compressStage(ColorSpace::FullSRGBGamma).value(),
            tiny_skia::pipeline::Stage::GammaCompressSrgb);

  const auto linearOpt = tiny_skia::Color::fromRgba(0.25f, 0.5f, 0.75f, 1.0f);
  ASSERT_TRUE(linearOpt.has_value());
  const auto color = *linearOpt;
  const auto expanded = tiny_skia::expandColor(ColorSpace::Linear, color);
  const std::array expandedChannels{expanded.red(), expanded.green(), expanded.blue(), expanded.alpha()};
  EXPECT_THAT(expandedChannels,
              ElementsAre(0.25f, 0.5f, 0.75f, 1.0f));

  const auto gammaCompressed =
      tiny_skia::compressChannel(ColorSpace::Gamma2,
                                tiny_skia::NormalizedF32::newUnchecked(0.25f));
  EXPECT_NEAR(gammaCompressed.get(), 0.5f, 0.0005f);
}

TEST(AlphaRunsTest, BasicsAndOperations) {
  tiny_skia::AlphaRuns runs(6);
  EXPECT_TRUE(runs.isEmpty());

  const auto midOffset = runs.add(0, 0, 2, 0, 50, 0);
  EXPECT_EQ(midOffset, 2u);
  EXPECT_FALSE(runs.isEmpty());
  EXPECT_EQ(runs.alpha[0], static_cast<std::uint8_t>(50));

  const auto stopOffset = runs.add(2, 0, 0, 8, 255, 0);
  EXPECT_EQ(stopOffset, 2u);
  EXPECT_EQ(runs.alpha[2], static_cast<std::uint8_t>(8));

  std::array<tiny_skia::AlphaRun, 6> runData{};
  std::array<std::uint8_t, 6> alphaData{};
  runData[0] = tiny_skia::AlphaRun{5};
  tiny_skia::AlphaRuns::breakRun(std::span{runData}, std::span{alphaData}, 2, 2);
  EXPECT_TRUE(runData[0].has_value());
  EXPECT_EQ(runData[0].value(), 2u);
  EXPECT_TRUE(runData[2].has_value());
  EXPECT_EQ(runData[2].value(), 2u);
  EXPECT_TRUE(runData[4].has_value());
  EXPECT_EQ(runData[4].value(), 1u);

  std::array<tiny_skia::AlphaRun, 6> breakAtRuns{};
  std::array<std::uint8_t, 6> breakAtAlpha{};
  breakAtRuns[0] = tiny_skia::AlphaRun{5};
  breakAtAlpha[0] = 11;
  tiny_skia::AlphaRuns::breakAt(std::span{breakAtAlpha},
                               std::span{breakAtRuns},
                               2);
  EXPECT_TRUE(breakAtRuns[0].has_value());
  EXPECT_EQ(breakAtRuns[0].value(), 2u);
  EXPECT_TRUE(breakAtRuns[2].has_value());
  EXPECT_EQ(breakAtRuns[2].value(), 3u);
  EXPECT_EQ(breakAtAlpha[2], breakAtAlpha[0]);
}
