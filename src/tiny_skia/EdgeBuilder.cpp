#include "tiny_skia/EdgeBuilder.h"

#include "tiny_skia/EdgeClipper.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <limits>

#include "tiny_skia/path64/Cubic64.h"

namespace tiny_skia {

namespace {

bool isFinite(Point point) {
  return std::isfinite(point.x) && std::isfinite(point.y);
}

bool isNotMonotonic(float a, float b, float c) {
  const auto ab = a - b;
  auto bc = b - c;
  if (ab < 0.0f) {
    bc = -bc;
  }
  return ab == 0.0f || bc < 0.0f;
}

void chopQuadAt(std::array<Point, 3> src, float t, std::array<Point, 5>& dst) {
  const auto p01 = Point{src[0].x + (src[1].x - src[0].x) * t,
                         src[0].y + (src[1].y - src[0].y) * t};
  const auto p12 = Point{src[1].x + (src[2].x - src[1].x) * t,
                         src[1].y + (src[2].y - src[1].y) * t};
  const auto p012 = Point{
      p01.x + (p12.x - p01.x) * t, p01.y + (p12.y - p01.y) * t};

  dst[0] = src[0];
  dst[1] = p01;
  dst[2] = p012;
  dst[3] = p12;
  dst[4] = src[2];
}

std::size_t chopQuadAtYExtrema(std::array<Point, 3> src, std::array<Point, 5>& dst) {
  const auto a = src[0].y;
  const auto b = src[1].y;
  const auto c = src[2].y;

  if (!isNotMonotonic(a, b, c)) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    return 0;
  }

  const auto numerator = static_cast<double>(a - b);
  const auto denominator = static_cast<double>(a - b - b + c);
  if (!std::isinf(denominator) && std::abs(denominator) >= std::numeric_limits<float>::epsilon()) {
    const auto t = numerator / denominator;
    if (t > 0.0 && t < 1.0) {
      chopQuadAt(src, static_cast<float>(t), dst);
      dst[1].x = dst[2].x;
      dst[3].x = dst[2].x;
      return 1;
    }
  }

  const auto bIsFirst = std::abs(a - b) < std::abs(b - c);
  src[1] = bIsFirst ? src[0] : src[2];
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  return 0;
}

void interpCubicAt(std::array<Point, 4> src, double t, std::array<Point, 7>& dst) {
  const auto ab = Point{src[0].x + static_cast<float>((src[1].x - src[0].x) * t),
                       src[0].y + static_cast<float>((src[1].y - src[0].y) * t)};
  const auto bc = Point{src[1].x + static_cast<float>((src[2].x - src[1].x) * t),
                       src[1].y + static_cast<float>((src[2].y - src[1].y) * t)};
  const auto cd = Point{src[2].x + static_cast<float>((src[3].x - src[2].x) * t),
                       src[2].y + static_cast<float>((src[3].y - src[2].y) * t)};
  const auto abc = Point{ab.x + static_cast<float>((bc.x - ab.x) * t),
                        ab.y + static_cast<float>((bc.y - ab.y) * t)};
  const auto bcd = Point{bc.x + static_cast<float>((cd.x - bc.x) * t),
                        bc.y + static_cast<float>((cd.y - bc.y) * t)};
  const auto abcd = Point{abc.x + static_cast<float>((bcd.x - abc.x) * t),
                         abc.y + static_cast<float>((bcd.y - abc.y) * t)};

  dst[0] = src[0];
  dst[1] = ab;
  dst[2] = abc;
  dst[3] = abcd;
  dst[4] = bcd;
  dst[5] = cd;
  dst[6] = src[3];
}

bool validUnitDivide(double numerator, double denominator, double& normalized) {
  if (denominator <= 0.0) {
    return false;
  }
  normalized = numerator / denominator;
  return normalized > 0.0 && normalized < 1.0;
}

std::size_t chopCubicAt(std::array<Point, 4> src,
                        std::span<const double> tValues,
                        std::array<Point, 10>& dst) {
  if (tValues.empty()) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    return 0;
  }

  std::size_t offset = 0;
  auto nextT = tValues[0];
  for (std::size_t i = 0; i < tValues.size(); ++i) {
    auto split = std::array<Point, 7>{};
    interpCubicAt(src, nextT, split);

    dst[offset] = split[0];
    dst[offset + 1] = split[1];
    dst[offset + 2] = split[2];
    dst[offset + 3] = split[3];

    if (i + 1 == tValues.size()) {
      break;
    }

    offset += 3;
    const auto diff = tValues[i + 1] - tValues[i];
    const auto base = 1.0 - tValues[i];
    if (!validUnitDivide(diff, base, nextT)) {
      dst[offset + 4] = split[6];
      dst[offset + 5] = split[6];
      dst[offset + 6] = split[6];
      break;
    }

    src = {split[3], split[4], split[5], split[6]};
  }

  return tValues.size();
}

std::size_t chopCubicAtYExtrema(std::array<Point, 4> src, std::array<Point, 10>& dst) {
  const auto axis = std::array<double, 8>{
      static_cast<double>(src[0].y), 0.0, static_cast<double>(src[1].y), 0.0,
      static_cast<double>(src[2].y), 0.0, static_cast<double>(src[3].y), 0.0};

  auto tValues = std::array<double, 6>{};
  auto rawCount = tiny_skia::path64::cubic64::findExtrema(axis, tValues);
  std::sort(tValues.begin(), tValues.begin() + static_cast<long>(rawCount));
  auto count = chopCubicAt(src, std::span<const double>{tValues.data(), rawCount}, dst);

  if (count > 0) {
    dst[2].y = dst[3].y;
    dst[4].y = dst[3].y;
    if (count == 2) {
      dst[5].y = dst[6].y;
      dst[7].y = dst[6].y;
    }
  }

  return count;
}

}  // namespace

std::optional<ShiftedIntRect> ShiftedIntRect::create(ScreenIntRect rect, std::int32_t shift) {
  const auto x = static_cast<std::int64_t>(rect.x());
  const auto y = static_cast<std::int64_t>(rect.y());
  const auto width = static_cast<std::int64_t>(rect.width());
  const auto height = static_cast<std::int64_t>(rect.height());

  if (shift < 0 || shift > 30) {
    return std::nullopt;
  }
  if ((x >> (63 - shift)) != 0 || (y >> (63 - shift)) != 0 ||
      (width >> (63 - shift)) != 0 || (height >> (63 - shift)) != 0) {
    return std::nullopt;
  }

  const auto shifted = ScreenIntRect::fromXYWH(static_cast<std::uint32_t>(x << shift),
                                             static_cast<std::uint32_t>(y << shift),
                                             static_cast<std::uint32_t>(width << shift),
                                             static_cast<std::uint32_t>(height << shift));
  if (!shifted.has_value()) {
    return std::nullopt;
  }
  return ShiftedIntRect{shifted.value(), shift};
}

const ScreenIntRect& ShiftedIntRect::shifted() const {
  return shiftedRect;
}

ScreenIntRect ShiftedIntRect::recover() const {
  return ScreenIntRect::fromXYWH(shiftedRect.x() >> shift,
                                shiftedRect.y() >> shift,
                                shiftedRect.widthSafe() >> shift,
                                shiftedRect.height() >> shift)
      .value();
}

BasicEdgeBuilder BasicEdgeBuilder::newBuilder(std::int32_t clipShift) {
  auto builder = BasicEdgeBuilder{};
  builder.clipShift_ = clipShift;
  return builder;
}

std::optional<std::vector<Edge>> BasicEdgeBuilder::buildEdges(const Path& path,
                                                             const ShiftedIntRect* clip,
                                                             std::int32_t clipShift) {
  auto builder = BasicEdgeBuilder::newBuilder(clipShift);
  if (!builder.build(path, clip, false)) {
    return std::nullopt;
  }
  if (builder.edgesCount() < 2) {
    return std::nullopt;
  }
  return std::move(builder.edges_);
}

bool BasicEdgeBuilder::build(const Path& path, const ShiftedIntRect* clip, bool canCullToTheRight) {
  if (clip != nullptr) {
    const auto clipRect = clip->recover().toRect();
    for (auto iterator = EdgeClipperIter(path, clipRect, canCullToTheRight);;) {
      const auto clippedEdges = iterator.next();
      if (!clippedEdges.has_value()) {
        break;
      }

      for (const auto& edgeValue : clippedEdges->span()) {
        if (!isFinite(edgeValue.points[0]) || !isFinite(edgeValue.points[1])) {
          return false;
        }

        switch (edgeValue.type) {
          case PathEdgeType::LineTo:
            pushLine(std::span<const Point, 2>{&edgeValue.points[0], 2});
            break;
          case PathEdgeType::QuadTo: {
            if (!isFinite(edgeValue.points[2])) {
              return false;
            }
            const auto points =
                std::array<Point, 3>{edgeValue.points[0], edgeValue.points[1], edgeValue.points[2]};
            pushQuad(std::span<const Point>{points.data(), 3});
            break;
          }
          case PathEdgeType::CubicTo: {
            if (!isFinite(edgeValue.points[2]) || !isFinite(edgeValue.points[3])) {
              return false;
            }
            const auto points = std::array<Point, 4>{edgeValue.points[0],
                                                    edgeValue.points[1],
                                                    edgeValue.points[2],
                                                    edgeValue.points[3]};
            pushCubic(std::span<const Point>{points.data(), 4});
            break;
          }
        }
      }
    }
    return true;
  }

  for (auto iterator = pathIter(path);; ) {
    auto edge = iterator.next();
    if (!edge.has_value()) {
      break;
    }

    const auto& edgeValue = edge.value();
    if (!isFinite(edgeValue.points[0]) || !isFinite(edgeValue.points[1])) {
      return false;
    }

    switch (edgeValue.type) {
      case PathEdgeType::LineTo:
        pushLine(std::span<const Point, 2>{&edgeValue.points[0], 2});
        break;
      case PathEdgeType::QuadTo: {
        if (!isFinite(edgeValue.points[2])) {
          return false;
        }
        const auto points =
            std::array<Point, 3>{edgeValue.points[0], edgeValue.points[1], edgeValue.points[2]};
        auto mono = std::array<Point, 5>{};
        const auto count = chopQuadAtYExtrema(points, mono);
        for (std::size_t i = 0; i <= count; ++i) {
          pushQuad(std::span<const Point>{mono.data() + i * 2, 3});
        }
        break;
      }
      case PathEdgeType::CubicTo: {
        if (!isFinite(edgeValue.points[2]) || !isFinite(edgeValue.points[3])) {
          return false;
        }
        const auto points = std::array<Point, 4>{edgeValue.points[0],
                                                edgeValue.points[1],
                                                edgeValue.points[2],
                                                edgeValue.points[3]};
        auto mono = std::array<Point, 10>{};
        const auto count = chopCubicAtYExtrema(points, mono);
        for (std::size_t i = 0; i <= count; ++i) {
          pushCubic(std::span<const Point>{mono.data() + i * 3, 4});
        }
        break;
      }
    }
  }

  return true;
}

std::size_t BasicEdgeBuilder::edgesCount() const {
  return edges_.size();
}

void BasicEdgeBuilder::clearEdges() {
  edges_.clear();
}

std::span<const Edge> BasicEdgeBuilder::edges() const {
  return edges_;
}

void BasicEdgeBuilder::pushLine(std::span<const Point, 2> points) {
  const auto edge = LineEdge::create(points[0], points[1], clipShift_);
  if (!edge.has_value()) {
    return;
  }

  auto combine = Combine::None;
  if (edge->isVertical() && !edges_.empty() && edges_.back().isLine()) {
    combine = combineVertical(*edge, edges_.back().asLine());
  }

  switch (combine) {
    case Combine::Total:
      edges_.pop_back();
      break;
    case Combine::Partial:
      break;
    case Combine::None:
      edges_.push_back(Edge(*edge));
      break;
  }
}

void BasicEdgeBuilder::pushQuad(std::span<const Point> points) {
  if (const auto edge = QuadraticEdge::create(points, clipShift_)) {
    edges_.push_back(Edge(*edge));
  }
}

void BasicEdgeBuilder::pushCubic(std::span<const Point> points) {
  if (const auto edge = CubicEdge::create(points, clipShift_)) {
    edges_.push_back(Edge(*edge));
  }
}

BasicEdgeBuilder::Combine BasicEdgeBuilder::combineVertical(const LineEdge& edge, LineEdge& last) {
  if (last.dx != 0 || edge.x != last.x) {
    return Combine::None;
  }

  if (edge.winding == last.winding) {
    if (edge.lastY + 1 == last.firstY) {
      last.firstY = edge.firstY;
      return Combine::Partial;
    }
    if (edge.firstY == last.lastY + 1) {
      last.lastY = edge.lastY;
      return Combine::Partial;
    }
    return Combine::None;
  }

  if (edge.firstY == last.firstY) {
    if (edge.lastY == last.lastY) {
      return Combine::Total;
    }
    if (edge.lastY < last.lastY) {
      last.firstY = edge.lastY + 1;
      return Combine::Partial;
    }
    last.firstY = last.lastY + 1;
    last.lastY = edge.lastY;
    last.winding = edge.winding;
    return Combine::Partial;
  }

  if (edge.lastY == last.lastY) {
    if (edge.firstY > last.firstY) {
      last.lastY = edge.firstY - 1;
    } else {
      last.lastY = last.firstY - 1;
      last.firstY = edge.firstY;
      last.winding = edge.winding;
    }
    return Combine::Partial;
  }

  return Combine::None;
}

PathEdgeIter::PathEdgeIter(const Path& path) : path_(&path) {}

std::optional<PathEdge> PathEdgeIter::closeLine() {
  if (pointsIndex_ == 0) {
    needsCloseLine_ = false;
    return std::nullopt;
  }

  needsCloseLine_ = false;
  auto edge = PathEdge{};
  edge.type = PathEdgeType::LineTo;
  edge.points[0] = path_->points()[pointsIndex_ - 1];
  edge.points[1] = moveTo_;
  return edge;
}

std::optional<PathEdge> PathEdgeIter::next() {
  const auto verbList = path_->verbs();
  const auto pointList = path_->points();

  while (verbIndex_ < verbList.size()) {
    const auto verb = verbList[verbIndex_];
    ++verbIndex_;

    if (verb == PathVerb::Move) {
      if (needsCloseLine_) {
        const auto close = closeLine();
        if (close.has_value()) {
          moveTo_ = pointList[pointsIndex_];
          ++pointsIndex_;
          return close;
        }
      }

      if (pointsIndex_ >= pointList.size()) {
        return std::nullopt;
      }
      moveTo_ = pointList[pointsIndex_];
      ++pointsIndex_;
      continue;
    }

    if (verb == PathVerb::Close) {
      if (needsCloseLine_) {
        return closeLine();
      }
      continue;
    }

    if ((verb == PathVerb::Line && pointsIndex_ >= pointList.size()) ||
        (verb == PathVerb::Quad && pointsIndex_ + 1 >= pointList.size()) ||
        (verb == PathVerb::Cubic && pointsIndex_ + 2 >= pointList.size())) {
      return std::nullopt;
    }

    needsCloseLine_ = true;
    if (verb == PathVerb::Line) {
      auto edge = PathEdge{};
      edge.type = PathEdgeType::LineTo;
      edge.points[0] = pointList[pointsIndex_ - 1];
      edge.points[1] = pointList[pointsIndex_];
      ++pointsIndex_;
      return edge;
    }

    if (verb == PathVerb::Quad) {
      auto edge = PathEdge{};
      edge.type = PathEdgeType::QuadTo;
      edge.points[0] = pointList[pointsIndex_ - 1];
      edge.points[1] = pointList[pointsIndex_];
      edge.points[2] = pointList[pointsIndex_ + 1];
      pointsIndex_ += 2;
      return edge;
    }

    auto edge = PathEdge{};
    edge.type = PathEdgeType::CubicTo;
    edge.points[0] = pointList[pointsIndex_ - 1];
    edge.points[1] = pointList[pointsIndex_];
    edge.points[2] = pointList[pointsIndex_ + 1];
    edge.points[3] = pointList[pointsIndex_ + 2];
    pointsIndex_ += 3;
    return edge;
  }

  if (needsCloseLine_) {
    return closeLine();
  }
  return std::nullopt;
}

PathEdgeIter pathIter(const Path& path) {
  return PathEdgeIter(path);
}

}  // namespace tiny_skia
