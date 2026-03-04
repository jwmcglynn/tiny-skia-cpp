/// Port of third_party/tiny-skia/examples/pattern.rs
///
/// Build and run:
///   bazel run //examples:pattern

#include <cstdio>

#include "PngEncoder.h"
#include "tiny_skia/Painter.h"
#include "tiny_skia/PathBuilder.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/shaders/Mod.h"

namespace {

tiny_skia::Pixmap createTriangle() {
  using namespace tiny_skia;

  Paint paint;
  paint.setColorRgba8(50, 127, 150, 200);
  paint.anti_alias = true;

  PathBuilder pb;
  pb.moveTo(0.0f, 20.0f);
  pb.lineTo(20.0f, 20.0f);
  pb.lineTo(10.0f, 0.0f);
  pb.close();
  auto path = pb.finish();

  auto pixmap = Pixmap::fromSize(20, 20);
  auto mut = pixmap->asMut();
  fillPath(mut, *path, paint, FillRule::Winding, Transform::identity());
  return std::move(*pixmap);
}

}  // namespace

int main() {
  using namespace tiny_skia;

  auto triangle = createTriangle();

  Paint paint;
  paint.anti_alias = true;
  paint.shader = Pattern(triangle.asRef(), SpreadMode::Repeat, FilterQuality::Bicubic, 1.0f,
                         Transform::fromRow(1.5f, -0.4f, 0.0f, -0.8f, 5.0f, 1.0f));

  auto path = PathBuilder::fromCircle(200.0f, 200.0f, 180.0f);

  auto pixmap = Pixmap::fromSize(400, 400);
  auto mut = pixmap->asMut();
  fillPath(mut, *path, paint, FillRule::Winding, Transform::identity());

  auto data = pixmap->takeDemultiplied();
  if (examples::writePng("pattern.png", data.data(), 400, 400)) {
    std::printf("Wrote pattern.png (400x400)\n");
  } else {
    std::fprintf(stderr, "Failed to write pattern.png\n");
    return 1;
  }
  return 0;
}
