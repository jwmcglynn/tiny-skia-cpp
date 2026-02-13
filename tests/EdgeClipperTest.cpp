#include <array>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/EdgeClipper.h"

namespace {

std::vector<tiny_skia::PathEdge> CollectClippedEdges(const tiny_skia::Path& path,
                                                    bool canCullToTheRight) {
  const auto clip = tiny_skia::Rect::fromLtrb(0.0f, 0.0f, 10.0f, 10.0f).value();
  auto iter = tiny_skia::EdgeClipperIter(path, clip, canCullToTheRight);

  auto result = std::vector<tiny_skia::PathEdge>{};
  for (auto edges = iter.next(); edges.has_value(); edges = iter.next()) {
    for (const auto& edge : edges->span()) {
      result.push_back(edge);
    }
  }
  return result;
}

}  // namespace

TEST(EdgeClipperTest, ClipLinePassesThroughWhenFullyInsideClip) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({5.0f, 5.0f});

  auto edges = CollectClippedEdges(path, true);
  ASSERT_EQ(edges.size(), 1u);
  EXPECT_EQ(edges.front().type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(edges.front().points[0].x, 2.0f);
  EXPECT_FLOAT_EQ(edges.front().points[0].y, 2.0f);
  EXPECT_FLOAT_EQ(edges.front().points[1].x, 5.0f);
  EXPECT_FLOAT_EQ(edges.front().points[1].y, 5.0f);
}

TEST(EdgeClipperTest, ClipLineCanCullToRightOrPreserveWhenDisabled) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({12.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 5.0f});

  EXPECT_TRUE(CollectClippedEdges(path, true).empty());

  const auto preserved = CollectClippedEdges(path, false);
  ASSERT_EQ(preserved.size(), 1u);
  EXPECT_EQ(preserved.front().type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(preserved.front().points[0].x, 10.0f);
  EXPECT_FLOAT_EQ(preserved.front().points[1].x, 10.0f);
  EXPECT_FLOAT_EQ(preserved.front().points[0].y, 2.0f);
  EXPECT_FLOAT_EQ(preserved.front().points[1].y, 5.0f);
}

TEST(EdgeClipperTest, ClipQuadFullyToLeftBecomesVerticalLine) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({-8.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({-6.0f, 5.0f});
  path.addPoint({-4.0f, 9.0f});

  auto edges = CollectClippedEdges(path, true);
  ASSERT_EQ(edges.size(), 1u);
  EXPECT_EQ(edges.front().type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(edges.front().points[0].x, 0.0f);
  EXPECT_FLOAT_EQ(edges.front().points[1].x, 0.0f);
}

TEST(EdgeClipperTest, ClipQuadFullyToRightProducesRightVerticalLineWhenCullingDisabled) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({12.0f, 1.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({14.0f, 4.0f});
  path.addPoint({16.0f, 8.0f});

  const auto culled = CollectClippedEdges(path, true);
  EXPECT_TRUE(culled.empty());

  auto edges = CollectClippedEdges(path, false);
  ASSERT_EQ(edges.size(), 1u);
  EXPECT_EQ(edges.front().type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(edges.front().points[0].x, 10.0f);
  EXPECT_FLOAT_EQ(edges.front().points[1].x, 10.0f);
}

TEST(EdgeClipperTest, ClipCubicPartiallyInsideProducesCubicAndBoundaryLines) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({-6.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Cubic);
  path.addPoint({2.0f, 2.0f});
  path.addPoint({8.0f, 8.0f});
  path.addPoint({12.0f, 8.0f});

  auto edges = CollectClippedEdges(path, true);
  ASSERT_GE(edges.size(), 1u);
  bool hasLeftBoundary = false;
  for (const auto& edge : edges) {
    if (edge.type == tiny_skia::PathEdgeType::LineTo && edge.points[0].x == 0.0f &&
        edge.points[1].x == 0.0f) {
      hasLeftBoundary = true;
      break;
    }
  }
  EXPECT_TRUE(hasLeftBoundary);
}

TEST(EdgeClipperTest, ClipVeryLargeCubicFallsBackToLineClipping) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({-8388608.0f, 2.0f});
  path.addVerb(tiny_skia::PathVerb::Cubic);
  path.addPoint({8388608.0f, 2.0f});
  path.addPoint({8388608.0f, 8.0f});
  path.addPoint({-8388608.0f, 8.0f});

  auto edges = CollectClippedEdges(path, true);
  ASSERT_GE(edges.size(), 1u);
  for (const auto& edge : edges) {
    EXPECT_EQ(edge.type, tiny_skia::PathEdgeType::LineTo);
    EXPECT_LE(edge.points[0].x, 10.0f);
    EXPECT_GE(edge.points[0].x, 0.0f);
    EXPECT_LE(edge.points[1].x, 10.0f);
    EXPECT_GE(edge.points[1].x, 0.0f);
  }
}

