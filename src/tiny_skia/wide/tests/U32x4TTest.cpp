#include "tiny_skia/wide/U32x4T.h"

#include <array>
#include <cstdint>

#include "gtest/gtest.h"

namespace {

using tiny_skia::wide::U32x4T;

TEST(U32x4TTest, CmpEqProducesAllOnesMaskPerLane) {
  const U32x4T lhs({1, 2, 3, 4});
  const U32x4T rhs({1, 9, 3, 0});

  EXPECT_EQ(lhs.cmpEq(rhs).lanes(),
            (std::array<std::uint32_t, 4>{UINT32_MAX, 0, UINT32_MAX, 0}));
}

TEST(U32x4TTest, ShiftOperationsMatchRustScalarFallback) {
  const U32x4T value({1, 2, 4, 8});
  EXPECT_EQ(value.shl<2>().lanes(), (std::array<std::uint32_t, 4>{4, 8, 16, 32}));
  EXPECT_EQ(value.shr<1>().lanes(), (std::array<std::uint32_t, 4>{0, 1, 2, 4}));
}

TEST(U32x4TTest, BitwiseAndAddOpsArePerLane) {
  const U32x4T lhs({0xFFFFFFFFu, 0x0F0F0F0Fu, 1u, 10u});
  const U32x4T rhs({1u, 0xF0F0F0F0u, 2u, 20u});

  EXPECT_EQ((~lhs).lanes(),
            (std::array<std::uint32_t, 4>{0u, 0xF0F0F0F0u, 0xFFFFFFFEu, 0xFFFFFFF5u}));
  EXPECT_EQ((lhs + rhs).lanes(),
            (std::array<std::uint32_t, 4>{0u, 0xFFFFFFFFu, 3u, 30u}));
  EXPECT_EQ((lhs & rhs).lanes(), (std::array<std::uint32_t, 4>{1u, 0u, 0u, 0u}));
  EXPECT_EQ((lhs | rhs).lanes(),
            (std::array<std::uint32_t, 4>{0xFFFFFFFFu, 0xFFFFFFFFu, 3u, 30u}));
}

}  // namespace
