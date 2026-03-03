/// Port of third_party/tiny-skia/examples/linear_gradient.rs
///
/// Build and run:
///   bazel run //examples:linear_gradient

#include <cstdio>
#include <variant>

#include "tiny_skia/Color.h"
#include "tiny_skia/Painter.h"
#include "tiny_skia/PathBuilder.h"
#include "tiny_skia/Pixmap.h"
#include "tiny_skia/shaders/Mod.h"

#include "PngEncoder.h"

int main() {
  using namespace tiny_skia;

  Paint paint;
  paint.anti_alias = false;

  auto result = LinearGradient::create(
      Point::fromXy(100.0f, 100.0f),
      Point::fromXy(900.0f, 900.0f),
      {
          GradientStop::create(0.0f, Color::fromRgba8(50, 127, 150, 200)),
          GradientStop::create(1.0f, Color::fromRgba8(220, 140, 75, 180)),
      },
      SpreadMode::Pad,
      Transform::identity());
  paint.shader = std::get<LinearGradient>(std::move(*result));

  PathBuilder pb;
  pb.moveTo(60.0f, 60.0f);
  pb.lineTo(160.0f, 940.0f);
  pb.cubicTo(380.0f, 840.0f, 660.0f, 800.0f, 940.0f, 800.0f);
  pb.cubicTo(740.0f, 460.0f, 440.0f, 160.0f, 60.0f, 60.0f);
  pb.close();
  auto path = pb.finish();

  auto pixmap = Pixmap::fromSize(1000, 1000);
  auto mut = pixmap->asMut();
  fillPath(mut, *path, paint, FillRule::Winding, Transform::identity());

  auto data = pixmap->takeDemultiplied();
  if (examples::writePng("linear_gradient.png", data.data(), 1000, 1000)) {
    std::printf("Wrote linear_gradient.png (1000x1000)\n");
  } else {
    std::fprintf(stderr, "Failed to write linear_gradient.png\n");
    return 1;
  }
  return 0;
}
