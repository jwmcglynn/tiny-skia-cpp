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

### `third_party/tiny-skia/src/shaders/sweep_gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `SweepGradient` struct | `SweepGradient` class | 🟡 | Contains Gradient base + t0/t1 angle parameters. Covered by `SweepGradientTest.*` (10 tests). |
| `SweepGradient::new` | `SweepGradient::create()` | 🟡 | Factory returns `optional<variant<Color, SweepGradient>>`. Handles empty/single stops, inverted/non-finite angles, degenerate (nearly-equal angles with hard-stop fallback), full-circle→Pad optimization. Covered by `SweepGradientTest.Create*`. |
| `SweepGradient::is_opaque` | `SweepGradient::isOpaque()` | 🟡 | Delegates to base Gradient. |
| `SweepGradient::push_stages` | `SweepGradient::pushStages()` | 🟡 | XYToUnitAngle + optional ApplyConcentricScaleBias (when partial sweep). Uses two_point_conical_gradient context for scale/bias. Covered by `SweepGradientTest.PushStages*`. |

### `third_party/tiny-skia/src/shaders/radial_gradient.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `FocalData` struct | `FocalData` struct | 🟡 | r1, focal_x, is_swapped fields. Methods: isFocalOnCircle, isWellBehaved, isNativelyFocal, set. |
| `FocalData::set` | `FocalData::set()` | 🟡 | Maps focal point to origin with tsFromPolyToPoly, applies scale for acceleration. Handles focal_x=1 edge case. |
| `GradientType` enum | `GradientType` (`variant<RadialType, StripType, FocalData>`) | 🟡 | Radial (simple/concentric), Strip (equal radii), Focal (two-point conical). |
| `RadialGradient` struct | `RadialGradient` class | 🟡 | Contains Gradient base + GradientType. Covered by `RadialGradientTest.*` (12 tests). |
| `RadialGradient::new` | `RadialGradient::create()` | 🟡 | Factory returns `optional<variant<Color, RadialGradient>>`. Handles empty/single stops, negative radii, non-invertible transform, degenerate same-center (→simple radial or hard-stop), same-center same-radii, different-center (→two-point conical or strip). Covered by `RadialGradientTest.Create*`. |
| `RadialGradient::new_radial_unchecked` | `RadialGradient::createRadialUnchecked()` | 🟡 | Optimized path for simple radial (center → radius). |
| `create` (module fn) | `RadialGradient::createTwoPoint()` | 🟡 | Full two-point conical logic: concentric, strip, and focal gradient type selection. |
| `RadialGradient::push_stages` | `RadialGradient::pushStages()` | 🟡 | Dispatches pre/post closures by GradientType: Radial→XYToRadius, Strip→XYTo2PtConicalStrip+masks, Focal→XYTo2PtConical{FocalOnCircle,WellBehaved,Smaller,Greater}+masks+compensate+unswap. Covered by `RadialGradientTest.PushStages*`. |
| `map_to_unit_x` | `mapToUnitX()` (anon namespace) | 🟡 | Internal helper mapping origin→x_is_one to unit X. |
| `ts_from_poly_to_poly` | `tsFromPolyToPoly()` | 🟡 | Transform mapping between two point pairs. Used by FocalData::set and mapToUnitX. |
| `from_poly2` | `fromPoly2()` (anon namespace) | 🟡 | Creates affine transform from two points. |

### `third_party/tiny-skia/src/shaders/pattern.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `FilterQuality` enum | `FilterQuality` enum | 🟡 | Nearest, Bilinear, Bicubic. |
| `PixmapPaint` struct | `PixmapPaint` struct | 🟡 | opacity, blend_mode, quality fields with defaults. |
| `Pattern` struct | `Pattern` class | 🟡 | pixmap, quality, spread_mode, opacity, transform. Covered by `PatternTest.*` (8 tests). |
| `Pattern::new` | `Pattern()` constructor | 🟡 | Takes PixmapRef, SpreadMode, FilterQuality, opacity, Transform. |
| `Pattern::push_stages` | `Pattern::pushStages()` | 🟡 | SeedShader → Transform → quality dispatch (Nearest: TileCtx+Gather, Bilinear: SamplerCtx+Bilinear, Bicubic: SamplerCtx+Bicubic+Clamp0+ClampA) → Scale1Float (if opacity<1) → expand stage. Quality downgrade for identity/translate transforms. Covered by `PatternTest.CreateAndPushStages*`. |

### `third_party/tiny-skia/path/src/scalar.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `SCALAR_NEARLY_ZERO` | `kScalarNearlyZero` (Math.h) | 🟡 | `1.0 / 4096.0`. Used by Transform and gradient logic. |
| `Scalar::is_nearly_zero` | `isNearlyZero()` (Math.h) | 🟡 | Inline function. |
| `Scalar::is_nearly_zero_within_tolerance` | `isNearlyZeroWithinTolerance()` (Math.h) | 🟡 | Inline function. Used by invDeterminant and degenerate checks. |
| `Scalar::is_nearly_equal` | `isNearlyEqual()` (Math.h) | 🟡 | Inline function. Used by Gradient uniform-stop detection. |
| `Scalar::invert` | `invert()` (Math.h) | 🟡 | `1.0f / value`. Used by Transform scale inversion and gradient normalization. |
| `Scalar::bound` | `bound()` (Math.h) | 🟡 | Templated `min(max, max(min, value))`. Used by gradient position clamping. |
