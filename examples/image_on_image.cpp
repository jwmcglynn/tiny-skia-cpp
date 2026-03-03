/// Port of third_party/tiny-skia/examples/image_on_image.rs
///
/// Build and run:
///   bazel run //examples:image_on_image

#include <cstdio>

#include "tiny_skia/Painter.h"
#include "tiny_skia/Path.h"
#include "tiny_skia/PathBuilder.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/Stroke.h"

#include "PngEncoder.h"

namespace {

tiny_skia::Pixmap createTriangle() {
  using namespace tiny_skia;

  Paint paint;
  paint.setColorRgba8(50, 127, 150, 200);
  paint.anti_alias = true;

  PathBuilder pb;
  pb.moveTo(0.0f, 200.0f);
  pb.lineTo(200.0f, 200.0f);
  pb.lineTo(100.0f, 0.0f);
  pb.close();
  auto path = pb.finish();

  auto pixmap = Pixmap::fromSize(200, 200);
  auto mut = pixmap->asMut();
  fillPath(mut, *path, paint, FillRule::Winding, Transform::identity());

  // Stroke a border around the triangle pixmap.
  auto rectPath = pathFromRect(*Rect::fromLtrb(0.0f, 0.0f, 200.0f, 200.0f));
  Stroke stroke;
  paint.setColorRgba8(200, 0, 0, 220);
  strokePath(mut, rectPath, paint, stroke, Transform::identity());

  return std::move(*pixmap);
}

}  // namespace

int main() {
  using namespace tiny_skia;

  auto triangle = createTriangle();

  auto pixmap = Pixmap::fromSize(400, 400);

  PixmapPaint ppaint;
  ppaint.quality = FilterQuality::Bicubic;

  auto mut = pixmap->asMut();
  drawPixmap(mut, 20, 20, triangle.asRef(), ppaint,
             Transform::fromRow(1.2f, 0.5f, 0.5f, 1.2f, 0.0f, 0.0f));

  auto data = pixmap->takeDemultiplied();
  if (examples::writePng("image_on_image.png", data.data(), 400, 400)) {
    std::printf("Wrote image_on_image.png (400x400)\n");
  } else {
    std::fprintf(stderr, "Failed to write image_on_image.png\n");
    return 1;
  }
  return 0;
}
