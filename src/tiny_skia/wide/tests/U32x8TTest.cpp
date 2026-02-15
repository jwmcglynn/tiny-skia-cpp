#include "tiny_skia/wide/U32x8T.h"

#include <array>
#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::ElementsAre;
using tiny_skia::wide::U32x8T;

TEST(U32x8TTest, CmpEqProducesAllOnesMaskPerLane) {
  const U32x8T lhs({1, 2, 3, 4, 5, 6, 7, 8});
  const U32x8T rhs({1, 9, 3, 0, 5, 0, 0, 8});

  EXPECT_THAT(lhs.cmpEq(rhs).lanes(),
              ElementsAre(UINT32_MAX, 0u, UINT32_MAX, 0u, UINT32_MAX, 0u, 0u, UINT32_MAX));
}

TEST(U32x8TTest, ShiftOperationsMatchRustScalarFallback) {
  const U32x8T value({1, 2, 4, 8, 16, 32, 64, 128});
  EXPECT_THAT(value.shl<2>().lanes(), ElementsAre(4u, 8u, 16u, 32u, 64u, 128u, 256u, 512u));
  EXPECT_THAT(value.shr<1>().lanes(), ElementsAre(0u, 1u, 2u, 4u, 8u, 16u, 32u, 64u));
}

TEST(U32x8TTest, BitwiseAndAddOpsArePerLane) {
  const U32x8T lhs({0xFFFFFFFFu, 0x0F0F0F0Fu, 1u, 10u, 100u, 1000u, 10000u, 100000u});
  const U32x8T rhs({1u, 0xF0F0F0F0u, 2u, 20u, 200u, 2000u, 20000u, 200000u});

  EXPECT_THAT((~lhs).lanes(),
              ElementsAre(0u, 0xF0F0F0F0u, 0xFFFFFFFEu, 0xFFFFFFF5u,
                          0xFFFFFF9Bu, 0xFFFFFC17u, 0xFFFFD8EFu, 0xFFFE795Fu));
  EXPECT_THAT((lhs + rhs).lanes(),
              ElementsAre(0u, 0xFFFFFFFFu, 3u, 30u, 300u, 3000u, 30000u, 300000u));
  EXPECT_THAT((lhs & rhs).lanes(),
              ElementsAre(1u, 0u, 0u, 0u, 64u, 960u, 1536u, 66560u));
  EXPECT_THAT((lhs | rhs).lanes(),
              ElementsAre(0xFFFFFFFFu, 0xFFFFFFFFu, 3u, 30u, 236u, 2040u, 28464u, 233440u));
}

}  // namespace
