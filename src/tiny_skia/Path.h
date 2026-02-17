#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "tiny_skia/Edge.h"
#include "tiny_skia/Geom.h"
#include "tiny_skia/pipeline/Mod.h"

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

class Path {
 public:
  Path() = default;
  Path(std::vector<PathVerb> verbs, std::vector<Point> points)
      : verbs_(std::move(verbs)), points_(std::move(points)) {
    recomputeBounds();
  }

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

}  // namespace tiny_skia
