#include <array>
#include <span>

#include <gtest/gtest.h>

#include "tiny_skia/Edge.h"

TEST(EdgeCubicTest, CubicEdgeCreateRejectsBadInputs) {
  const std::array<tiny_skia::Point, 3> badCubic{{{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f}}};
  EXPECT_FALSE(
      tiny_skia::CubicEdge::create(std::span{badCubic.data(), badCubic.size()}, 0).has_value());
}

TEST(EdgeCubicTest, CubicEdgeCreateBasic) {
  const std::array<tiny_skia::Point, 4> cubic{
      {{0.0f, 0.0f}, {2.0f, 0.0f}, {4.0f, 6.0f}, {6.0f, 6.0f}}};

  const auto edgeOpt = tiny_skia::CubicEdge::create(
      std::span{cubic.data(), cubic.size()}, 0);
  ASSERT_TRUE(edgeOpt.has_value());
  const auto& edge = edgeOpt.value();
  EXPECT_LT(edge.curveCount, 0);
  EXPECT_NE(edge.cLastX, edge.cx);
  EXPECT_NE(edge.cLastY, edge.cy);
}

TEST(EdgeCubicTest, CubicEdgeCreateDescendingYFlipsWindingAndUpdates) {
  const std::array<tiny_skia::Point, 4> cubic{
      {{0.0f, 8.0f}, {2.0f, 6.0f}, {4.0f, 4.0f}, {6.0f, 2.0f}}};

  auto edgeOpt = tiny_skia::CubicEdge::create(
      std::span{cubic.data(), cubic.size()}, 0);
  ASSERT_TRUE(edgeOpt.has_value());
  auto edge = edgeOpt.value();
  EXPECT_EQ(edge.line.winding, -1);

  const auto xBefore = edge.cx;
  const auto yBefore = edge.cy;
  (void)edge.update();
  EXPECT_NE(edge.cx, xBefore);
  EXPECT_NE(edge.cy, yBefore);
}

