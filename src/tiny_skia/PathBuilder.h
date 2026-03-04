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

  void moveTo(float x, float y);
  void lineTo(float x, float y);
  void quadTo(float x1, float y1, float x, float y);
  void quadToPt(Point p1, Point p);
  void cubicTo(float x1, float y1, float x2, float y2, float x, float y);
  void cubicToPt(Point p1, Point p2, Point p);
  void conicTo(float x1, float y1, float x, float y, float weight);
  void conicPointsTo(Point pt1, Point pt2, float weight);
  void close();

  [[nodiscard]] std::optional<Point> lastPoint() const;
  void setLastPoint(Point pt);

  [[nodiscard]] bool isZeroLengthSincePoint(std::size_t startPtIndex) const;

  /// Creates a new Path from a circle.
  [[nodiscard]] static std::optional<Path> fromCircle(float cx, float cy, float radius) {
    PathBuilder b;
    b.pushCircle(cx, cy, radius);
    return b.finish();
  }

  void pushRect(const Rect& rect);
  void pushOval(const Rect& oval);
  void pushCircle(float x, float y, float r);
  void pushPath(const Path& other);
  void pushPathBuilder(const PathBuilder& other);
  void reversePathTo(const PathBuilder& other);

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
