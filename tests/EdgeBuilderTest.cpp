#include <algorithm>
#include <limits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "tiny_skia/EdgeBuilder.h"
#include "tiny_skia/Geom.h"

namespace {

TEST(EdgeBuilderTest, BuildEdgesRejectsEmptyPath) {
  tiny_skia::Path path;
  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  EXPECT_FALSE(edges.has_value());
}

TEST(EdgeBuilderTest, BuildEdgesBuildsSimpleTriangle) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({0.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({10.0f, 10.0f});
  path.addVerb(tiny_skia::PathVerb::Close);

  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  ASSERT_TRUE(edges.has_value());
  ASSERT_GE(edges->size(), 2u);
  EXPECT_TRUE(std::all_of(edges->begin(),
                          edges->end(),
                          [](const auto& edge) { return edge.isLine(); }));
}

TEST(EdgeBuilderTest, BuildEdgesBuildsMixedEdgesForMonotonicQuad) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({0.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Quad);
  path.addPoint({10.0f, 10.0f});
  path.addPoint({20.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({20.0f, 10.0f});
  path.addVerb(tiny_skia::PathVerb::Close);

  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  ASSERT_TRUE(edges.has_value());
  EXPECT_GE(edges->size(), 2u);
  const auto hasQuadratic = std::any_of(edges->begin(), edges->end(), [](const auto& edge) {
    return edge.isQuadratic();
  });
  const auto hasLine = std::any_of(edges->begin(), edges->end(), [](const auto& edge) {
    return edge.isLine();
  });
  EXPECT_TRUE(hasQuadratic);
  EXPECT_TRUE(hasLine);
}

TEST(EdgeBuilderTest, BuildEdgesAutoClosesTrailingOpenContour) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({2.0f, 1.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({7.0f, 3.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({9.0f, 5.0f});

  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  ASSERT_TRUE(edges.has_value());
  ASSERT_GE(edges->size(), 2u);
  EXPECT_TRUE(edges->front().isLine());
  EXPECT_TRUE(edges->back().isLine());
}

TEST(EdgeBuilderTest, BuildEdgesRejectsMalformedPath) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({0.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);

  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  EXPECT_FALSE(edges.has_value());
}

TEST(EdgeBuilderTest, BuildEdgesRejectsNonFiniteInputs) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({0.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({std::numeric_limits<float>::quiet_NaN(), 10.0f});

  const auto edges = tiny_skia::BasicEdgeBuilder::buildEdges(path, nullptr, 0);
  EXPECT_FALSE(edges.has_value());
}

TEST(EdgeBuilderTest, PathIterClosesOpenContourBeforeMoveVerb) {
  tiny_skia::Path path;
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({0.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({10.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Move);
  path.addPoint({20.0f, 0.0f});
  path.addVerb(tiny_skia::PathVerb::Line);
  path.addPoint({30.0f, 10.0f});

  auto iter = tiny_skia::pathIter(path);
  auto first = iter.next();
  auto second = iter.next();
  auto third = iter.next();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  ASSERT_TRUE(third.has_value());

  EXPECT_EQ(first->type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(first->points[0].x, 0.0f);
  EXPECT_FLOAT_EQ(first->points[0].y, 0.0f);
  EXPECT_FLOAT_EQ(first->points[1].x, 10.0f);
  EXPECT_FLOAT_EQ(first->points[1].y, 0.0f);

  EXPECT_EQ(second->type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(second->points[0].x, 10.0f);
  EXPECT_FLOAT_EQ(second->points[0].y, 0.0f);
  EXPECT_FLOAT_EQ(second->points[1].x, 0.0f);
  EXPECT_FLOAT_EQ(second->points[1].y, 0.0f);

  EXPECT_EQ(third->type, tiny_skia::PathEdgeType::LineTo);
  EXPECT_FLOAT_EQ(third->points[0].x, 20.0f);
  EXPECT_FLOAT_EQ(third->points[0].y, 0.0f);
  EXPECT_FLOAT_EQ(third->points[1].x, 30.0f);
  EXPECT_FLOAT_EQ(third->points[1].y, 10.0f);
}

TEST(EdgeBuilderTest, ShiftedIntRectCreateRoundTrips) {
  const auto source = tiny_skia::ScreenIntRect::fromXYWH(1, 2, 10, 20).value();
  const auto shifted = tiny_skia::ShiftedIntRect::create(source, 2).value();

  const auto recovered = shifted.recover();
  EXPECT_EQ(shifted.shifted().x(), 4u);
  EXPECT_EQ(shifted.shifted().y(), 8u);
  EXPECT_EQ(shifted.shifted().width(), 40u);
  EXPECT_EQ(shifted.shifted().height(), 80u);

  EXPECT_EQ(recovered.x(), source.x());
  EXPECT_EQ(recovered.y(), source.y());
  EXPECT_EQ(recovered.width(), source.width());
  EXPECT_EQ(recovered.height(), source.height());
}

TEST(EdgeBuilderTest, ShiftedIntRectCreateRejectsBadShifts) {
  const auto source = tiny_skia::ScreenIntRect::fromXYWH(1, 2, 10, 20).value();
  EXPECT_FALSE(tiny_skia::ShiftedIntRect::create(source, -1).has_value());
  EXPECT_FALSE(tiny_skia::ShiftedIntRect::create(source, 31).has_value());
}

}  // namespace
