#include "tiny_skia/wide/F32x8T.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tiny_skia/wide/I32x8T.h"

namespace {

using ::testing::ElementsAre;
using tiny_skia::wide::F32x8T;

TEST(F32x8TTest, AbsMinMaxAndNormalizeArePerLane) {
  const F32x8T value({-1.5f, 2.0f, -0.0f, -99.25f, 3.0f, -4.0f, 5.5f, 2.5f});
  EXPECT_THAT(value.abs().lanes(),
              ElementsAre(1.5f, 2.0f, 0.0f, 99.25f, 3.0f, 4.0f, 5.5f, 2.5f));

  const F32x8T lhs({1.0f, -3.0f, 5.0f, 4.0f, -10.0f, 6.0f, 7.0f, 8.0f});
  const F32x8T rhs({2.0f, -4.0f, 3.0f, 10.0f, 1.0f, 5.0f, 9.0f, -8.0f});
  EXPECT_THAT(lhs.min(rhs).lanes(),
              ElementsAre(1.0f, -4.0f, 3.0f, 4.0f, -10.0f, 5.0f, 7.0f, -8.0f));
  EXPECT_THAT(lhs.max(rhs).lanes(),
              ElementsAre(2.0f, -3.0f, 5.0f, 10.0f, 1.0f, 6.0f, 9.0f, 8.0f));

  EXPECT_THAT(F32x8T({-1.0f, 0.2f, 1.5f, 2.0f, -0.3f, 0.7f, 1.0f, 0.0f}).normalize().lanes(),
              ElementsAre(0.0f, 0.2f, 1.0f, 1.0f, 0.0f, 0.7f, 1.0f, 0.0f));
}

TEST(F32x8TTest, ComparisonsAndBlendUseMaskBits) {
  const F32x8T lhs({1.0f, 2.0f, 3.0f, 4.0f, 4.0f, 6.0f, 7.0f, 8.0f});
  const F32x8T rhs({1.0f, 0.0f, 5.0f, 4.0f, 5.0f, 5.0f, 7.0f, 9.0f});

  const auto eq = lhs.cmpEq(rhs).lanes();
  EXPECT_EQ(std::bit_cast<std::uint32_t>(eq[0]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(eq[1]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(eq[3]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(eq[7]), 0u);

  const F32x8T mask({std::bit_cast<float>(0xFFFFFFFFu), 0.0f, std::bit_cast<float>(0xFFFFFFFFu),
                     0.0f, std::bit_cast<float>(0xFFFFFFFFu), 0.0f,
                     std::bit_cast<float>(0xFFFFFFFFu), 0.0f});
  const F32x8T onTrue({10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f});
  const F32x8T onFalse({1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f});
  EXPECT_THAT(mask.blend(onTrue, onFalse).lanes(),
              ElementsAre(10.0f, 2.0f, 30.0f, 4.0f, 50.0f, 6.0f, 70.0f, 8.0f));
}

TEST(F32x8TTest, FloorFractRoundAndIntegerConversionsMatchScalarFallback) {
  const F32x8T value({1.9f, -1.1f, 2.5f, -2.5f, 0.0f, 3.99f, -3.99f, 8.01f});

  EXPECT_THAT(value.floor().lanes(),
              ElementsAre(1.0f, -2.0f, 2.0f, -3.0f, 0.0f, 3.0f, -4.0f, 8.0f));

  const auto fract = value.fract().lanes();
  EXPECT_FLOAT_EQ(fract[0], 0.9f);
  EXPECT_FLOAT_EQ(fract[1], 0.9f);
  EXPECT_FLOAT_EQ(fract[2], 0.5f);
  EXPECT_FLOAT_EQ(fract[3], 0.5f);

  EXPECT_THAT(value.round().lanes(),
              ElementsAre(2.0f, -1.0f, 2.0f, -2.0f, 0.0f, 4.0f, -4.0f, 8.0f));

  const tiny_skia::wide::I32x8T rounded = value.roundInt();
  EXPECT_THAT(rounded.lanes(), ElementsAre(2, -1, 2, -2, 0, 4, -4, 8));

  const tiny_skia::wide::I32x8T truncated = value.truncInt();
  EXPECT_THAT(truncated.lanes(), ElementsAre(1, -1, 2, -2, 0, 3, -3, 8));
}

TEST(F32x8TTest, IsFiniteMatchesRustBitMaskLogic) {
  const F32x8T value({0.0f,
                      std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN(),
                      1.0f,
                      -42.0f,
                      3.14f,
                      -0.0f});

  const auto mask = value.isFinite().lanes();
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[0]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[1]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[2]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[3]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[4]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[5]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[6]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(mask[7]), 0xFFFFFFFFu);
}

}  // namespace
