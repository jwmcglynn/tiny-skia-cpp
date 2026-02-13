#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "tiny_skia/Edge.h"

namespace tiny_skia {

enum class PathVerb : std::uint8_t {
  Move,
  Close,
  Line,
  Quad,
  Cubic,
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

 private:
  std::vector<PathVerb> verbs_;
  std::vector<Point> points_;
};

}  // namespace tiny_skia
