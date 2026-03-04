#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "tiny_skia/Path.h"
#include "tiny_skia/PathGeometry.h"
#include "tiny_skia/Scalar.h"

namespace tiny_skia {

/// A path builder.
class PathBuilder {
 public:
  PathBuilder() = default;

  PathBuilder(std::size_t verbsCapacity, std::size_t pointsCapacity) {
    verbs_.reserve(verbsCapacity);
    points_.reserve(pointsCapacity);
  }

  void reserve(std::size_t additionalVerbs, std::size_t additionalPoints) {
    verbs_.reserve(verbs_.size() + additionalVerbs);
    points_.reserve(points_.size() + additionalPoints);
  }

  [[nodiscard]] std::size_t size() const { return verbs_.size(); }
  [[nodiscard]] bool empty() const { return verbs_.empty(); }

  PathBuilder& moveTo(float x, float y);
  PathBuilder& lineTo(float x, float y);
  PathBuilder& quadTo(float x1, float y1, float x, float y);
  PathBuilder& quadToPt(Point p1, Point p);
  PathBuilder& cubicTo(float x1, float y1, float x2, float y2, float x, float y);
  PathBuilder& cubicToPt(Point p1, Point p2, Point p);
  PathBuilder& conicTo(float x1, float y1, float x, float y, float weight);
  PathBuilder& conicPointsTo(Point pt1, Point pt2, float weight);
  PathBuilder& close();

  [[nodiscard]] std::optional<Point> lastPoint() const;
  void setLastPoint(Point pt);

  [[nodiscard]] bool isZeroLengthSincePoint(std::size_t startPtIndex) const;

  /// Creates a new Path from a circle.
  [[nodiscard]] static std::optional<Path> fromCircle(float cx, float cy, float radius) {
    PathBuilder b;
    b.pushCircle(cx, cy, radius);
    return b.finish();
  }

  PathBuilder& pushRect(const Rect& rect);
  PathBuilder& pushOval(const Rect& oval);
  PathBuilder& pushCircle(float x, float y, float r);
  PathBuilder& pushPath(const Path& other);
  PathBuilder& pushPathBuilder(const PathBuilder& other);
  PathBuilder& reversePathTo(const PathBuilder& other);

  void clear();

  [[nodiscard]] std::optional<Path> finish();

  // Access for internal use by stroker.
  [[nodiscard]] const std::vector<Point>& points() const { return points_; }
  [[nodiscard]] const std::vector<PathVerb>& verbs() const { return verbs_; }

 private:
  void injectMoveToIfNeeded();

  std::vector<PathVerb> verbs_;
  std::vector<Point> points_;
  std::size_t lastMoveToIndex_ = 0;
  bool moveToRequired_ = true;
};

}  // namespace tiny_skia
