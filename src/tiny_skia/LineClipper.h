#pragma once

#include <array>
#include <span>

#include "tiny_skia/Edge.h"
#include "tiny_skia/Geom.h"

namespace tiny_skia {

namespace line_clipper {

constexpr int kLineClipperMaxPoints = 4;

std::span<const Point> clip(std::span<const Point, 2> src, const Rect& clip, bool canCullToTheRight,
                            std::span<Point, kLineClipperMaxPoints> pointsOut);

bool intersect(std::span<const Point, 2> src, const Rect& clip, std::span<Point, 2> dst);

}  // namespace line_clipper

}  // namespace tiny_skia
