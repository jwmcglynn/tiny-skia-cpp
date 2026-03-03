#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "tiny_skia/Edge.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/Transform.h"

namespace tiny_skia {

enum class PathVerb : std::uint8_t {
  Move,
  Close,
  Line,
  Quad,
  Cubic,
};

enum class LineCap : std::uint8_t {
  Butt,
  Round,
  Square,
};

// Forward declarations for stroke/dash.
class PathBuilder;
struct Stroke;
struct StrokeDash;

/// A path segment for iteration. Matches Rust `PathSegment`.
struct PathSegment {
  enum class Kind : std::uint8_t { MoveTo, LineTo, QuadTo, CubicTo, Close };
  Kind kind;
  Point pts[3] = {};  // up to 3 points depending on kind
};

class Path {
 public:
  Path() = default;
  Path(std::vector<PathVerb> verbs, std::vector<Point> points)
      : verbs_(std::move(verbs)), points_(std::move(points)) {
    recomputeBounds();
  }

  [[nodiscard]] std::size_t len() const { return verbs_.size(); }
  [[nodiscard]] bool isEmpty() const { return verbs_.empty(); }

  [[nodiscard]] std::span<const PathVerb> verbs() const {
    return verbs_;
  }
  [[nodiscard]] std::span<const Point> points() const {
    return points_;
  }

  void addVerb(PathVerb verb) {
    verbs_.push_back(verb);
  }

  void addPoint(Point point) {
    if (bounds_.has_value()) {
      const auto current = bounds_.value();
      bounds_ = Rect::fromLtrb(std::min(current.left(), point.x),
                               std::min(current.top(), point.y),
                               std::max(current.right(), point.x),
                               std::max(current.bottom(), point.y));
    } else {
      bounds_ = Rect::fromLtrb(point.x, point.y, point.x, point.y);
    }
    points_.push_back(point);
  }

  [[nodiscard]] bool isConvex() const {
    return true;
  }

  [[nodiscard]] Rect bounds() const {
    return bounds_.value_or(Rect::fromLtrb(0.0f, 0.0f, 0.0f, 0.0f).value());
  }

  /// Returns a new Path with all points transformed.
  /// Returns nullopt if the transform produces non-finite values.
  [[nodiscard]] std::optional<Path> transform(const Transform& ts) const {
    auto pts = points_;
    ts.mapPoints(pts);
    for (const auto& p : pts) {
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        return std::nullopt;
      }
    }
    return Path(verbs_, std::move(pts));
  }

  /// Computes tight bounds by finding curve extrema.
  /// Unlike bounds() which uses control points, this finds exact extrema.
  /// Matches Rust `Path::compute_tight_bounds`.
  [[nodiscard]] std::optional<Rect> computeTightBounds() const;

  /// Clears the path and returns a PathBuilder reusing the allocations.
  /// Matches Rust `Path::clear`. Consumes (moves from) this path.
  [[nodiscard]] PathBuilder clear();

  /// Stroke this path. Returns a filled path representing the stroke outline.
  [[nodiscard]] std::optional<Path> stroke(const Stroke& stroke,
                                            float resScale) const;

  /// Dash this path. Returns a new path with dash pattern applied.
  [[nodiscard]] std::optional<Path> dash(const StrokeDash& dash,
                                          float resScale) const;

 private:
  void recomputeBounds() {
    if (points_.empty()) {
      bounds_.reset();
      return;
    }

    auto left = points_[0].x;
    auto top = points_[0].y;
    auto right = points_[0].x;
    auto bottom = points_[0].y;

    for (const auto& point : points_) {
      left = std::min(left, point.x);
      right = std::max(right, point.x);
      top = std::min(top, point.y);
      bottom = std::max(bottom, point.y);
    }

    bounds_ = Rect::fromLtrb(left, top, right, bottom);
  }

  std::vector<PathVerb> verbs_;
  std::vector<Point> points_;
  std::optional<Rect> bounds_;
};

enum class FillRule : std::uint8_t {
  Winding = 0,
  EvenOdd = 1,
};

/// Creates a rectangular path from a Rect.
/// Matches Rust `PathBuilder::from_rect`.
inline Path pathFromRect(const Rect& rect) {
  std::vector<PathVerb> verbs = {
      PathVerb::Move, PathVerb::Line, PathVerb::Line, PathVerb::Line,
      PathVerb::Close};
  std::vector<Point> points = {
      Point{rect.left(), rect.top()},
      Point{rect.right(), rect.top()},
      Point{rect.right(), rect.bottom()},
      Point{rect.left(), rect.bottom()}};
  return Path(std::move(verbs), std::move(points));
}

/// Path segments iterator. Matches Rust `PathSegmentsIter`.
class PathSegmentsIter {
 public:
  explicit PathSegmentsIter(const Path& path)
      : path_(&path) {}

  void setAutoClose(bool flag) { isAutoClose_ = flag; }

  std::optional<PathSegment> next();

  [[nodiscard]] Point lastPoint() const { return lastPoint_; }
  [[nodiscard]] Point lastMoveTo() const { return lastMoveTo_; }

  [[nodiscard]] PathVerb currVerb() const {
    return path_->verbs()[verbIndex_ - 1];
  }

  [[nodiscard]] std::optional<PathVerb> nextVerb() const {
    if (verbIndex_ < path_->verbs().size()) {
      return path_->verbs()[verbIndex_];
    }
    return std::nullopt;
  }

  [[nodiscard]] bool hasValidTangent() const;

 private:
  PathSegment autoClose();

  const Path* path_;
  std::size_t verbIndex_ = 0;
  std::size_t pointsIndex_ = 0;
  bool isAutoClose_ = false;
  Point lastMoveTo_ = Point::zero();
  Point lastPoint_ = Point::zero();
};

}  // namespace tiny_skia
