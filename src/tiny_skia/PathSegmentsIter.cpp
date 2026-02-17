#include "tiny_skia/Path.h"

namespace tiny_skia {

PathSegment PathSegmentsIter::autoClose() {
  if (isAutoClose_ && lastPoint_ != lastMoveTo_) {
    verbIndex_--;
    PathSegment seg;
    seg.kind = PathSegment::Kind::LineTo;
    seg.pts[0] = lastMoveTo_;
    return seg;
  }
  PathSegment seg;
  seg.kind = PathSegment::Kind::Close;
  return seg;
}

std::optional<PathSegment> PathSegmentsIter::next() {
  auto verbs = path_->verbs();
  auto pts = path_->points();

  if (verbIndex_ >= verbs.size()) {
    return std::nullopt;
  }

  PathVerb verb = verbs[verbIndex_];
  verbIndex_++;

  PathSegment seg;
  switch (verb) {
    case PathVerb::Move: {
      pointsIndex_++;
      lastMoveTo_ = pts[pointsIndex_ - 1];
      lastPoint_ = lastMoveTo_;
      seg.kind = PathSegment::Kind::MoveTo;
      seg.pts[0] = lastMoveTo_;
      break;
    }
    case PathVerb::Line: {
      pointsIndex_++;
      lastPoint_ = pts[pointsIndex_ - 1];
      seg.kind = PathSegment::Kind::LineTo;
      seg.pts[0] = lastPoint_;
      break;
    }
    case PathVerb::Quad: {
      pointsIndex_ += 2;
      lastPoint_ = pts[pointsIndex_ - 1];
      seg.kind = PathSegment::Kind::QuadTo;
      seg.pts[0] = pts[pointsIndex_ - 2];
      seg.pts[1] = lastPoint_;
      break;
    }
    case PathVerb::Cubic: {
      pointsIndex_ += 3;
      lastPoint_ = pts[pointsIndex_ - 1];
      seg.kind = PathSegment::Kind::CubicTo;
      seg.pts[0] = pts[pointsIndex_ - 3];
      seg.pts[1] = pts[pointsIndex_ - 2];
      seg.pts[2] = lastPoint_;
      break;
    }
    case PathVerb::Close: {
      seg = autoClose();
      lastPoint_ = lastMoveTo_;
      break;
    }
  }

  return seg;
}

bool PathSegmentsIter::hasValidTangent() const {
  // Clone iterator state.
  PathSegmentsIter iter = *this;
  while (auto seg = iter.next()) {
    switch (seg->kind) {
      case PathSegment::Kind::MoveTo:
        return false;
      case PathSegment::Kind::LineTo:
        if (iter.lastPoint_ == seg->pts[0]) continue;
        return true;
      case PathSegment::Kind::QuadTo:
        if (iter.lastPoint_ == seg->pts[0] &&
            iter.lastPoint_ == seg->pts[1])
          continue;
        return true;
      case PathSegment::Kind::CubicTo:
        if (iter.lastPoint_ == seg->pts[0] &&
            iter.lastPoint_ == seg->pts[1] &&
            iter.lastPoint_ == seg->pts[2])
          continue;
        return true;
      case PathSegment::Kind::Close:
        return false;
    }
  }
  return false;
}

}  // namespace tiny_skia
