#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <gtest/gtest.h>

#include "tiny_skia/AlphaRuns.h"

TEST(AlphaRunsTest, CatchOverflow) {
  EXPECT_EQ(tiny_skia::AlphaRuns::catchOverflow(0), 0u);
  EXPECT_EQ(tiny_skia::AlphaRuns::catchOverflow(1), 1u);
  EXPECT_EQ(tiny_skia::AlphaRuns::catchOverflow(128), 128u);
  EXPECT_EQ(tiny_skia::AlphaRuns::catchOverflow(255), 255u);
  EXPECT_EQ(tiny_skia::AlphaRuns::catchOverflow(256), 255u);
}

TEST(AlphaRunsTest, ConstructorResetAndIsEmpty) {
  tiny_skia::AlphaRuns runs(5);
  EXPECT_TRUE(runs.isEmpty());
  EXPECT_TRUE(runs.runs[0].has_value());
  EXPECT_EQ(*runs.runs[0], 5u);
  EXPECT_EQ(runs.alpha[0], 0u);
  EXPECT_FALSE(runs.runs[5].has_value());

  runs.runs = std::vector<tiny_skia::AlphaRun>(6);
  runs.alpha = std::vector<std::uint8_t>(6, 42);
  runs.reset(7);
  EXPECT_TRUE(runs.runs[0].has_value());
  EXPECT_EQ(*runs.runs[0], 7u);
  EXPECT_EQ(runs.alpha[0], 0u);
}

TEST(AlphaRunsTest, AddComposesStartAndMiddle) {
  tiny_skia::AlphaRuns runs(5);
  auto offset = runs.add(1, 0, 3, 0, 70, 0);
  EXPECT_EQ(offset, 4u);
  EXPECT_FALSE(runs.isEmpty());

  EXPECT_TRUE(runs.runs[0].has_value());
  EXPECT_EQ(*runs.runs[0], 1u);
  EXPECT_TRUE(runs.runs[1].has_value());
  EXPECT_EQ(*runs.runs[1], 3u);
  EXPECT_TRUE(runs.runs[4].has_value());
  EXPECT_EQ(*runs.runs[4], 1u);
  EXPECT_EQ(runs.alpha[1], 70u);
}

TEST(AlphaRunsTest, AddUsesOffsetAndStopAlpha) {
  tiny_skia::AlphaRuns runs(6);
  const auto first = runs.add(1, 0, 3, 0, 70, 0);
  EXPECT_EQ(first, 4u);
  EXPECT_TRUE(runs.runs[1].has_value());
  EXPECT_EQ(*runs.runs[1], 3u);
  EXPECT_EQ(runs.alpha[1], 70u);

  const auto second = runs.add(4, 0, 0, 15, 0, first);
  EXPECT_EQ(second, 4u);
  EXPECT_EQ(runs.alpha[4], 15u);
}

TEST(AlphaRunsTest, BreakRunSplitsRunsWithNonZeroBoundaries) {
  std::array<tiny_skia::AlphaRun, 8> runs{};
  std::array<std::uint8_t, 8> alpha{};
  runs[0] = tiny_skia::AlphaRun{8};
  alpha[0] = 11;

  tiny_skia::AlphaRuns::breakRun(std::span{runs}, std::span{alpha}, 0, 3);
  EXPECT_TRUE(runs[0].has_value());
  EXPECT_EQ(*runs[0], 3u);
  EXPECT_TRUE(runs[3].has_value());
  EXPECT_EQ(*runs[3], 5u);
  EXPECT_EQ(alpha[3], 11u);
  EXPECT_FALSE(runs[6].has_value());
}

TEST(AlphaRunsTest, BreakAtSplitsAtOffset) {
  std::array<tiny_skia::AlphaRun, 8> runs{};
  std::array<std::uint8_t, 8> alpha{};
  runs[0] = tiny_skia::AlphaRun{8};
  alpha[0] = 11;

  tiny_skia::AlphaRuns::breakAt(std::span{alpha}, std::span{runs}, 2);
  EXPECT_TRUE(runs[0].has_value());
  EXPECT_EQ(*runs[0], 2u);
  EXPECT_TRUE(runs[2].has_value());
  EXPECT_EQ(*runs[2], 6u);
  EXPECT_EQ(alpha[2], 11u);
}
