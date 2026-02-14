#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tiny_skia {

class Color;
class PremultipliedColor;
class NormalizedF32;
class PixmapRef;
class ScreenIntRect;
struct SubPixmapMut;

class Transform {
 public:
  constexpr Transform() = default;
  constexpr Transform(bool is_finite, bool is_identity)
      : is_finite_(is_finite), is_identity_(is_identity) {}

  [[nodiscard]] constexpr bool isFinite() const {
    return is_finite_;
  }
  [[nodiscard]] constexpr bool isIdentity() const {
    return is_identity_;
  }

 private:
  bool is_finite_ = true;
  bool is_identity_ = true;
};

enum class SpreadMode {
  Pad,
  Reflect,
  Repeat,
};

namespace pipeline {

enum class Stage : std::uint8_t {
  MoveSourceToDestination,
  MoveDestinationToSource,
  Clamp0,
  ClampA,
  Premultiply,
  UniformColor,
  SeedShader,
  LoadDestination,
  Store,
  LoadDestinationU8,
  StoreU8,
  Gather,
  LoadMaskU8,
  MaskU8,
  ScaleU8,
  LerpU8,
  Scale1Float,
  Lerp1Float,
  DestinationAtop,
  DestinationIn,
  DestinationOut,
  DestinationOver,
  SourceAtop,
  SourceIn,
  SourceOut,
  SourceOver,
  Clear,
  Modulate,
  Multiply,
  Plus,
  Screen,
  Xor,
  ColorBurn,
  ColorDodge,
  Darken,
  Difference,
  Exclusion,
  HardLight,
  Lighten,
  Overlay,
  SoftLight,
  Hue,
  Saturation,
  Color,
  Luminosity,
  SourceOverRgba,
  Transform,
  Reflect,
  Repeat,
  Bilinear,
  Bicubic,
  PadX1,
  ReflectX1,
  RepeatX1,
  Gradient,
  EvenlySpaced2StopGradient,
  XYToUnitAngle,
  XYToRadius,
  XYTo2PtConicalFocalOnCircle,
  XYTo2PtConicalWellBehaved,
  XYTo2PtConicalSmaller,
  XYTo2PtConicalGreater,
  XYTo2PtConicalStrip,
  Mask2PtConicalNan,
  Mask2PtConicalDegenerates,
  ApplyVectorMask,
  Alter2PtConicalCompensateFocal,
  Alter2PtConicalUnswap,
  NegateX,
  ApplyConcentricScaleBias,
  GammaExpand2,
  GammaExpandDestination2,
  GammaCompress2,
  GammaExpand22,
  GammaExpandDestination22,
  GammaCompress22,
  GammaExpandSrgb,
  GammaExpandDestinationSrgb,
  GammaCompressSrgb,
};

inline constexpr std::size_t kStagesCount = 1 + static_cast<std::size_t>(Stage::GammaCompressSrgb);
inline constexpr std::size_t kMaxStages = 32;

struct AAMaskCtx {
  std::array<std::uint8_t, 2> pixels = {0, 0};
  std::uint32_t stride = 0;  // can be zero
  std::size_t shift = 0;     // mask offset/position in pixmap coordinates

  [[nodiscard]] std::array<std::uint8_t, 2> copyAtXY(std::size_t dx,
                                                     std::size_t dy,
                                                     std::size_t tail) const {
    const auto base = static_cast<std::size_t>(stride) * dy + dx;
    if (base < shift) {
      return {0, 0};
    }
    const auto offset = base - shift;
    if (offset == 0 && tail == 1) {
      return {pixels[0], 0};
    }
    if (offset == 0 && tail == 2) {
      return {pixels[0], pixels[1]};
    }
    if (offset == 1 && tail == 1) {
      return {pixels[1], 0};
    }
    return {0, 0};
  }
};

struct MaskCtx {
  const std::uint8_t* data = nullptr;
  std::uint32_t real_width = 0;

 private:
  [[nodiscard]] constexpr std::size_t offset(std::size_t dx, std::size_t dy) const {
    return static_cast<std::size_t>(real_width) * dy + dx;
  }
};

struct SamplerCtx {
  SpreadMode spread_mode = SpreadMode::Pad;
  float inv_width = 0.0f;
  float inv_height = 0.0f;
};

struct UniformColorCtx {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
  std::array<std::uint16_t, 4> rgba = {0, 0, 0, 0};  // [0,255] in a 16-bit lane.
};

struct GradientColor {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;

  constexpr bool operator==(const GradientColor&) const = default;

  [[nodiscard]] static constexpr GradientColor newFromRGBA(float r, float g, float b, float a) {
    return GradientColor{r, g, b, a};
  }
};

struct EvenlySpaced2StopGradientCtx {
  GradientColor factor{};
  GradientColor bias{};
};

struct TwoPointConicalGradientCtx {
  // This context is used only in high precision.
  std::uint32_t mask = 0;
  float p0 = 0.0f;
  float p1 = 0.0f;
};

struct TileCtx {
  float scale = 0.0f;
  float inv_scale = 0.0f;  // cache of 1/scale
};

struct Context {
  float current_coverage = 0.0f;
  SamplerCtx sampler;
  UniformColorCtx uniform_color;
  EvenlySpaced2StopGradientCtx evenly_spaced_2_stop_gradient;
  struct GradientCtx {
    std::size_t len = 0;
    std::vector<GradientColor> factors;
    std::vector<GradientColor> biases;
    std::vector<float> t_values;

    void pushConstColor(GradientColor color) {
      factors.push_back(GradientColor{0.0f, 0.0f, 0.0f, 0.0f});
      biases.push_back(color);
    }
  } gradient;

  TwoPointConicalGradientCtx two_point_conical_gradient;
  TileCtx limit_x;
  TileCtx limit_y;
  Transform transform;
};

class RasterPipeline {
 public:
  enum class Kind {
    High,
    Low,
  };

  RasterPipeline() = default;

  RasterPipeline(Kind kind, Context context) : kind_(kind), ctx_(context) {}

  [[nodiscard]] Kind kind() const {
    return kind_;
  }

  Context& ctx() {
    return ctx_;
  }
  [[nodiscard]] const Context& ctx() const {
    return ctx_;
  }

  void run(const ScreenIntRect& /*rect*/,
           AAMaskCtx /*aa_mask_ctx*/,
           MaskCtx /*mask_ctx*/,
           const PixmapRef& /*pixmap_src*/,
           SubPixmapMut* /*pixmap_dst*/) {}

 private:
  Kind kind_ = Kind::High;
  Context ctx_{};
};

class RasterPipelineBuilder {
 public:
  RasterPipelineBuilder() = default;

  void setForceHqPipeline(bool hq) {
    force_hq_pipeline_ = hq;
  }

  void push(Stage stage) {
    if (stage_count_ >= kMaxStages) {
      return;
    }
    stages_[stage_count_++] = stage;
  }

  void pushTransform(const Transform& ts) {
    if (ts.isFinite() && !ts.isIdentity()) {
      push(Stage::Transform);
      ctx_.transform = ts;
    }
  }

  void pushUniformColor(const PremultipliedColor& c);

  [[nodiscard]] RasterPipeline compile();

  [[nodiscard]] Context& ctx() {
    return ctx_;
  }
  [[nodiscard]] const Context& ctx() const {
    return ctx_;
  }

 private:
  std::array<Stage, kMaxStages> stages_ = {};
  std::size_t stage_count_ = 0;
  bool force_hq_pipeline_ = false;
  Context ctx_;
};

}  // namespace pipeline

}  // namespace tiny_skia
