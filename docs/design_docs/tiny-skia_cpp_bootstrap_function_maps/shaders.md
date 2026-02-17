# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/shaders/mod.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `SpreadMode` | `SpreadMode` (in pipeline/Mod.h) | 🟡 | Enum with Pad, Reflect, Repeat variants. Used by Gradient and LinearGradient. |
| `Shader` enum | `Shader` (`std::variant<Color, LinearGradient>`) | 🟡 | Variant type. RadialGradient/SweepGradient/Pattern deferred. Covered by `ShaderTest.*` tests. |
| `Shader::is_opaque` | `isShaderOpaque()` | 🟡 | Free function dispatching via std::visit. Covered by `ShaderTest.SolidColorIsOpaque`, `SolidColorTransparentIsNotOpaque`, `LinearGradientOpaqueIsOpaque`. |
| `Shader::push_stages` | `pushShaderStages()` | 🟡 | Free function. SolidColor→pushUniformColor, LinearGradient→pushStages. Covered by `ShaderTest.PushStagesSolidColor` and `LinearGradientTest.PushStages*`. |
| `Shader::transform` | `transformShader()` | 🟡 | Free function. SolidColor is no-op, LinearGradient updates base_.transform via postConcat. Covered by `ShaderTest.TransformShaderSolidColorIsNoOp`, `TransformShaderUpdatesGradientTransform`. |
| `Shader::apply_opacity` | `applyShaderOpacity()` | 🟡 | Free function. Delegates to Color::applyOpacity or Gradient::applyOpacity. Covered by `ShaderTest.ApplyOpacitySolidColor`, `ApplyOpacityGradient`. |

### `third_party/tiny-skia/src/shaders/gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `DEGENERATE_THRESHOLD` | `kDegenerateThreshold` | 🟡 | `1.0f / (1 << 15)`. Used in LinearGradient::create for degenerate detection. |
| `GradientStop` | `GradientStop` | 🟡 | Struct with NormalizedF32 position and Color. Factory `create()` clamps. Covered by `GradientStopTest.*`. |
| `Gradient::new` | `Gradient::Gradient` (constructor) | 🟡 | Dummy endpoint insertion, monotonic position enforcement, uniform-stop detection, opacity tracking. Exercised through all LinearGradient creation tests. |
| `Gradient::push_stages` | `Gradient::pushStages()` | 🟡 | SeedShader → Transform → TileMode → (2-stop optimization \| multi-stop GradientCtx) → Premultiply. Covered by `LinearGradientTest.PushStages*`. |
| `Gradient::apply_opacity` | `Gradient::applyOpacity()` | 🟡 | Iterates stops, applies opacity, rechecks colors_are_opaque. Covered by `ShaderTest.ApplyOpacityGradient`. |

### `third_party/tiny-skia/src/shaders/linear_gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `LinearGradient::new` | `LinearGradient::create()` | 🟡 | Factory returns `optional<variant<Color, LinearGradient>>`. Handles empty stops, single stop (→SolidColor), degenerate (Pad→last color, Repeat/Reflect→average), non-invertible transform, infinite length. Covered by `LinearGradientTest.Create*`. |
| `LinearGradient::push_stages` | `LinearGradient::pushStages()` | 🟡 | Delegates to base Gradient::pushStages with no pre/post stage callbacks. Covered by `LinearGradientTest.PushStages*`. |
| `points_to_unit_ts` | `pointsToUnitTs()` (anonymous namespace) | 🟡 | Converts gradient start/end points to unit-space transform. Internal helper. |
| `average_gradient_color` | `averageGradientColor()` (anonymous namespace) | 🟡 | Weighted color average for degenerate gradients. Covered by `LinearGradientTest.CreateDegenerateRepeatReturnsAverage`. |

### `third_party/tiny-skia/path/src/transform.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Transform` struct | `Transform` class (pipeline/Mod.h) | 🟡 | 6-field affine matrix (sx, kx, ky, sy, tx, ty). Covered by `TransformTest.*` (15 tests). |
| `Transform::identity` | `Transform::identity()` | 🟡 | Covered by `TransformTest.IdentityIsIdentity`. |
| `Transform::from_row` | `Transform::fromRow()` | 🟡 | Covered by `TransformTest.FromRowWithSkew`. |
| `Transform::from_translate` | `Transform::fromTranslate()` | 🟡 | Covered by `TransformTest.FromTranslateClassification`. |
| `Transform::from_scale` | `Transform::fromScale()` | 🟡 | Covered by `TransformTest.FromScaleClassification`, `FromScaleOneIsNotScale`. |
| `Transform::is_identity` | `Transform::isIdentity()` | 🟡 | Covered by `TransformTest.IdentityIsIdentity`. |
| `Transform::is_translate` | `Transform::isTranslate()` | 🟡 | Covered by `TransformTest.IdentityIsIdentity`, `FromTranslateClassification`. |
| `Transform::is_scale_translate` | `Transform::isScaleTranslate()` | 🟡 | Covered by `TransformTest.IdentityIsIdentity`, `FromScaleClassification`, `FromTranslateClassification`. |
| `Transform::has_scale` | `Transform::hasScale()` | 🟡 | Covered by `TransformTest.*`. |
| `Transform::has_skew` | `Transform::hasSkew()` | 🟡 | Covered by `TransformTest.FromRowWithSkew`. |
| `Transform::has_translate` | `Transform::hasTranslate()` | 🟡 | Covered by `TransformTest.*`. |
| `Transform::invert` | `Transform::invert()` | 🟡 | f64-precision determinant. Identity/scale-translate fast paths, singular detection. Covered by `TransformTest.InvertIdentity`, `InvertScale`, `InvertSingularReturnsNullopt`, `InvertRoundTrip`. |
| `Transform::pre_concat` | `Transform::preConcat()` | 🟡 | Covered by `TransformTest.PreConcatIdentity`. |
| `Transform::post_concat` | `Transform::postConcat()` | 🟡 | Covered by `TransformTest.PostConcatIdentity`, `InvertRoundTrip`. |
| `Transform::pre_scale` | `Transform::preScale()` | 🟡 | Covered by `TransformTest.PreScale` (matches Rust concat test values). |
| `Transform::post_scale` | `Transform::postScale()` | 🟡 | Covered by `TransformTest.PostScale` (matches Rust concat test values). |
| `Transform::pre_translate` | `Transform::preTranslate()` | 🟡 | Covered by `TransformTest.PreTranslate`. |
| `Transform::post_translate` | `Transform::postTranslate()` | 🟡 | Covered by `TransformTest.PostTranslate`. |

### `third_party/tiny-skia/src/shaders/radial_gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `RadialGradient` | — | ☐ | Not yet ported. |

### `third_party/tiny-skia/src/shaders/sweep_gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `SweepGradient` | — | ☐ | Not yet ported. |

### `third_party/tiny-skia/src/shaders/pattern.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Pattern` | — | ☐ | Not yet ported. |

### `third_party/tiny-skia/path/src/scalar.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `SCALAR_NEARLY_ZERO` | `kScalarNearlyZero` (Math.h) | 🟡 | `1.0 / 4096.0`. Used by Transform and gradient logic. |
| `Scalar::is_nearly_zero` | `isNearlyZero()` (Math.h) | 🟡 | Inline function. |
| `Scalar::is_nearly_zero_within_tolerance` | `isNearlyZeroWithinTolerance()` (Math.h) | 🟡 | Inline function. Used by invDeterminant and degenerate checks. |
| `Scalar::is_nearly_equal` | `isNearlyEqual()` (Math.h) | 🟡 | Inline function. Used by Gradient uniform-stop detection. |
| `Scalar::invert` | `invert()` (Math.h) | 🟡 | `1.0f / value`. Used by Transform scale inversion and gradient normalization. |
| `Scalar::bound` | `bound()` (Math.h) | 🟡 | Templated `min(max, max(min, value))`. Used by gradient position clamping. |
