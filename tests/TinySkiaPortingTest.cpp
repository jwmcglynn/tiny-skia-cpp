#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/AlphaRuns.h"
#include "tiny_skia/Color.h"

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

TEST(ColorTest, ColorFromRgbaEdgeCases) {
  EXPECT_TRUE(tiny_skia::Color::fromRgba(0.0f, 0.0f, 0.0f, 0.0f).has_value());
  EXPECT_TRUE(tiny_skia::Color::fromRgba(1.0f, 1.0f, 1.0f, 1.0f).has_value());
  EXPECT_FALSE(
      tiny_skia::Color::fromRgba(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 0.0f)
          .has_value())
      << "NaN should be rejected";
  EXPECT_FALSE(
      tiny_skia::Color::fromRgba(std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f)
          .has_value())
      << "infinity should be rejected";
}

TEST(ColorTest, ColorSettersClampToRange) {
  auto color = tiny_skia::Color::fromRgba8(0, 0, 0, 255);
  color.setRed(-1.0f);
  color.setGreen(1.5f);
  color.setBlue(-0.2f);
  color.setAlpha(2.0f);
  expectColorFloatIs(color, 0.0f, 1.0f, 0.0f, 1.0f);
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
