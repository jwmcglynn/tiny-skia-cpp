#include <array>
#include <span>

#include <gtest/gtest.h>

#include "tiny_skia/Edge.h"

TEST(EdgeQuadraticTest, QuadraticEdgeCreateRejectsBadInputs) {
  const std::array<tiny_skia::Point, 2> badQuad{{{0.0f, 0.0f}, {1.0f, 1.0f}}};
  EXPECT_FALSE(tiny_skia::QuadraticEdge::create(
      std::span{badQuad.data(), badQuad.size()}, 0).has_value());
}

TEST(EdgeQuadraticTest, QuadraticEdgeCreateBasic) {
  const std::array<tiny_skia::Point, 3> quad{{{0.0f, 0.0f}, {2.0f, 4.0f}, {4.0f, 6.0f}}};

  const auto edgeOpt = tiny_skia::QuadraticEdge::create(
      std::span{quad.data(), quad.size()}, 0);
  ASSERT_TRUE(edgeOpt.has_value());
  const auto& edge = edgeOpt.value();
  EXPECT_TRUE(edge.line.firstY < edge.line.lastY);
  EXPECT_GT(edge.curveCount, 0);
  EXPECT_NE(edge.qLastX, edge.qx);
  EXPECT_NE(edge.qLastY, edge.qy);
}

TEST(EdgeQuadraticTest, QuadraticEdgeCreateDescendingYFlipsWinding) {
  const std::array<tiny_skia::Point, 3> quad{{{0.0f, 5.0f}, {2.0f, 3.0f}, {4.0f, 1.0f}}};

  const auto edgeOpt = tiny_skia::QuadraticEdge::create(
      std::span{quad.data(), quad.size()}, 0);
  ASSERT_TRUE(edgeOpt.has_value());
  EXPECT_EQ(edgeOpt.value().line.winding, -1);
}

TEST(EdgeQuadraticTest, QuadraticEdgeUpdateCanAdvance) {
  const std::array<tiny_skia::Point, 3> quad{{{0.0f, 0.0f}, {1.0f, 2.0f}, {2.0f, 4.0f}}};

  auto edgeOpt = tiny_skia::QuadraticEdge::create(
      std::span{quad.data(), quad.size()}, 0);
  ASSERT_TRUE(edgeOpt.has_value());

  auto edge = edgeOpt.value();
  const auto xBefore = edge.qx;
  const auto yBefore = edge.qy;

  (void)edge.update();

  EXPECT_NE(edge.qx, xBefore);
  EXPECT_NE(edge.qy, yBefore);
}

