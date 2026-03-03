#pragma once

#include <variant>

#include "tiny_skia/shaders/Gradient.h"
#include "tiny_skia/shaders/LinearGradient.h"
#include "tiny_skia/shaders/Pattern.h"
#include "tiny_skia/shaders/RadialGradient.h"
#include "tiny_skia/shaders/SweepGradient.h"

namespace tiny_skia {

// The Shader variant type. Matches Rust `enum Shader`.
// Uses std::variant to dispatch between shader types.
using Shader = std::variant<Color, LinearGradient, SweepGradient, RadialGradient, Pattern>;

// Returns true if the shader is guaranteed to produce only opaque colors.
[[nodiscard]] bool isShaderOpaque(const Shader& shader);

// Pushes shader stages into the pipeline builder.
// Returns false if the shader fails (e.g., non-invertible transform).
[[nodiscard]] bool pushShaderStages(const Shader& shader, ColorSpace cs,
                                    pipeline::RasterPipelineBuilder& p);

// Applies a transform to the shader.
void transformShader(Shader& shader, const Transform& ts);

// Applies opacity to the shader.
void applyShaderOpacity(Shader& shader, float opacity);

}  // namespace tiny_skia
