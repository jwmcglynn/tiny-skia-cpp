#include <cstdint>
#include <limits>
#include <string_view>

#include <gtest/gtest.h>

#include "tiny_skia/FixedPoint.h"

TEST(FixedPointFdot6Test, ConversionsAndRounding) {
  EXPECT_EQ(tiny_skia::fdot6::fromI32(3), 192);
  EXPECT_EQ(tiny_skia::fdot6::fromI32(-2), -128);
  EXPECT_EQ(tiny_skia::fdot6::fromF32(1.5f), 96);
  EXPECT_EQ(tiny_skia::fdot6::fromF32(-1.25f), -80);
  EXPECT_EQ(tiny_skia::fdot6::floor(-65), -2);
  EXPECT_EQ(tiny_skia::fdot6::ceil(-65), -1);
  EXPECT_EQ(tiny_skia::fdot6::round(-65), -1);
  EXPECT_EQ(tiny_skia::fdot6::floor(65), 1);
  EXPECT_EQ(tiny_skia::fdot6::ceil(65), 2);
  EXPECT_EQ(tiny_skia::fdot6::round(31), 0);
  EXPECT_EQ(tiny_skia::fdot6::toFdot16(64), 65536);
  EXPECT_EQ(tiny_skia::fdot6::div(64, 32), 131072);
  EXPECT_EQ(tiny_skia::fdot6::smallScale(255, 64), 255);
  EXPECT_EQ(tiny_skia::fdot6::smallScale(255, 32), 127);

  struct CanConvertCase {
    std::string_view label;
    std::int32_t value;
    bool expected;
  };
  for (const auto& tc : {
           CanConvertCase{"safe bound", 30000, true},
           CanConvertCase{"safe max", std::numeric_limits<std::int32_t>::max() >> 10, true},
           CanConvertCase{
               "overflow bound", (std::numeric_limits<std::int32_t>::max() >> 10) + 1, false},
           CanConvertCase{"overflow lower", std::numeric_limits<std::int32_t>::min(), false},
       }) {
    SCOPED_TRACE(tc.label);
    EXPECT_EQ(tiny_skia::fdot6::canConvertToFdot16(tc.value), tc.expected);
  }

  // Trigger the slow path when left-shifted fdot6 does not fit int16.
  const auto big = tiny_skia::fdot6::fromF32(3000.0f);
  EXPECT_EQ(tiny_skia::fdot16::divide(big, 64), 196608000);
}

TEST(FixedPointFdot6And8Test, Fdot8ConversionAndBoundaries) {
  EXPECT_EQ(tiny_skia::fdot8::fromFdot16(0xFF00), 0x0FF);
  EXPECT_EQ(tiny_skia::fdot8::fromFdot16(-0x0100), static_cast<tiny_skia::FDot8>(-0x01));
  EXPECT_EQ(tiny_skia::fdot8::fromFdot16(-0x7F00), -0x7F);
}

TEST(FixedPointFdot16Test, FloatingConversionAndArithmetic) {
  EXPECT_EQ(tiny_skia::fdot16::fromF32(1.0f), 65536);
  EXPECT_EQ(tiny_skia::fdot16::fromF32(-1.0f), -65536);
  EXPECT_EQ(tiny_skia::fdot16::fromF32(40000.0f), std::numeric_limits<std::int32_t>::max());
  EXPECT_EQ(tiny_skia::fdot16::fromF32(-40000.0f), std::numeric_limits<std::int32_t>::min());

  EXPECT_EQ(tiny_skia::fdot16::floorToI32(65535), 0);
  EXPECT_EQ(tiny_skia::fdot16::floorToI32(-65535), -1);
  EXPECT_EQ(tiny_skia::fdot16::ceilToI32(65535), 1);
  EXPECT_EQ(tiny_skia::fdot16::ceilToI32(-65535), 0);
  EXPECT_EQ(tiny_skia::fdot16::roundToI32(98304), 2);
  EXPECT_EQ(tiny_skia::fdot16::roundToI32(-98304), -1);

  EXPECT_EQ(tiny_skia::fdot16::mul(65536, 65536), 65536);
  EXPECT_EQ(tiny_skia::fdot16::mul(65536, -32768), -32768);

  EXPECT_EQ(tiny_skia::fdot16::divide(65536, 1), std::numeric_limits<std::int32_t>::max());
  EXPECT_EQ(tiny_skia::fdot16::divide(65536, -1), std::numeric_limits<std::int32_t>::min());
  EXPECT_EQ(tiny_skia::fdot16::fastDiv(64, 32), 131072);
  EXPECT_EQ(tiny_skia::fdot16::fastDiv(-64, 32), -131072);

  EXPECT_EQ(tiny_skia::fdot16::divide(std::numeric_limits<tiny_skia::FDot6>::max(), 1),
            std::numeric_limits<std::int32_t>::max());
  EXPECT_EQ(tiny_skia::fdot16::divide(std::numeric_limits<tiny_skia::FDot6>::min(), 1),
            std::numeric_limits<std::int32_t>::min());
}

TEST(Fdot16AndFdot8, DivisionAndRoundingEdgeCases) {
  EXPECT_EQ(tiny_skia::fdot16::divide(0, 1), 0);
  EXPECT_EQ(tiny_skia::fdot16::fastDiv(1, 1), 65536);
  EXPECT_EQ(tiny_skia::fdot16::fastDiv(0, 255), 0);

  struct CeilBoundaryCase {
    std::int32_t fixed;
    std::int32_t expected;
  };
  for (const auto& tc : {
           CeilBoundaryCase{1, 1},
           CeilBoundaryCase{65535, 1},
           CeilBoundaryCase{-65535, 0},
       }) {
    EXPECT_EQ(tiny_skia::fdot16::ceilToI32(tc.fixed), tc.expected);
  }
}

TEST(Fdot16AndFdot8, OneAndConstants) {
  EXPECT_EQ(tiny_skia::fdot16::one, 65536);
}
