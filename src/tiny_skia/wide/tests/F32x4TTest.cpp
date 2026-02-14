#include "tiny_skia/wide/F32x4T.h"

#include <array>
#include <bit>
#include <cstdint>

#include "gtest/gtest.h"

namespace {

using tiny_skia::wide::F32x4T;

TEST(F32x4TTest, AbsClearsSignBitPerLane) {
  const F32x4T value({-1.5f, 2.0f, -0.0f, -99.25f});
  EXPECT_EQ(value.abs().lanes(), (std::array<float, 4>{1.5f, 2.0f, 0.0f, 99.25f}));
}

TEST(F32x4TTest, MinAndMaxUsePerLaneComparisons) {
  const F32x4T lhs({1.0f, -3.0f, 5.0f, 4.0f});
  const F32x4T rhs({2.0f, -4.0f, 3.0f, 10.0f});

  EXPECT_EQ(lhs.min(rhs).lanes(), (std::array<float, 4>{1.0f, -4.0f, 3.0f, 4.0f}));
  EXPECT_EQ(lhs.max(rhs).lanes(), (std::array<float, 4>{2.0f, -3.0f, 5.0f, 10.0f}));
}

TEST(F32x4TTest, ComparisonsReturnAllOnesMaskOrZero) {
  const F32x4T lhs({1.0f, 2.0f, 3.0f, 4.0f});
  const F32x4T rhs({1.0f, 0.0f, 5.0f, 4.0f});
  const auto result = lhs.cmpEq(rhs).lanes();

  EXPECT_EQ(std::bit_cast<std::uint32_t>(result[0]), 0xFFFFFFFFu);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(result[1]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(result[2]), 0u);
  EXPECT_EQ(std::bit_cast<std::uint32_t>(result[3]), 0xFFFFFFFFu);
}

TEST(F32x4TTest, BlendSelectsTrueAndFalseLanesByMaskBits) {
  const F32x4T mask(
      {std::bit_cast<float>(0xFFFFFFFFu), 0.0f, std::bit_cast<float>(0xFFFFFFFFu), 0.0f});
  const F32x4T onTrue({10.0f, 20.0f, 30.0f, 40.0f});
  const F32x4T onFalse({1.0f, 2.0f, 3.0f, 4.0f});

  EXPECT_EQ(mask.blend(onTrue, onFalse).lanes(),
            (std::array<float, 4>{10.0f, 2.0f, 30.0f, 4.0f}));
}

}  // namespace
