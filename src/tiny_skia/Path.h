#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "tiny_skia/Edge.h"
#include "tiny_skia/Geom.h"

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
      : verbs_(std::move(verbs)), points_(std::move(points)) {}

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
    points_.push_back(point);
  }

  [[nodiscard]] bool isConvex() const {
    return true;
  }

  [[nodiscard]] Rect bounds() const {
    if (points_.empty()) {
      return Rect::fromLtrb(0.0f, 0.0f, 0.0f, 0.0f).value();
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

    return Rect::fromLtrb(left, top, right, bottom).value();
  }

 private:
  std::vector<PathVerb> verbs_;
  std::vector<Point> points_;
};

enum class FillRule : std::uint8_t {
  Winding = 0,
  EvenOdd = 1,
};

}  // namespace tiny_skia
