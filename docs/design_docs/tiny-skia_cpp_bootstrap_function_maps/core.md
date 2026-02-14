# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/lib.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| _none tracked_ | _none_ | ⏸ | No function-level symbols tracked for this Rust file in current C++ scope |

### `third_party/tiny-skia/src/alpha_runs.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `AlphaRuns::new` | `AlphaRuns::AlphaRuns` | 🟢 | Line-by-line audited: allocates `width+1` runs/alpha and calls `reset(width)` like Rust |
| `AlphaRuns::catch_overflow` | `AlphaRuns::catchOverflow` | 🟢 | Line-by-line audited: identical `alpha - (alpha >> 8)` overflow catch and `alpha <= 256` guard |
| `AlphaRuns::is_empty` | `AlphaRuns::isEmpty` | 🟢 | Line-by-line audited: same `runs[0]`/`alpha[0]` and terminal-run-empty logic |
| `AlphaRuns::reset` | `AlphaRuns::reset` | 🟢 | Line-by-line audited: sets `runs[0]`, `runs[width]=None/nullopt`, and `alpha[0]=0` equivalently |
| `AlphaRuns::add` | `AlphaRuns::add` | 🟢 | Line-by-line audited: start/middle/stop branches, offset updates, and overflow handling match Rust flow |
| `AlphaRuns::break_run` | `AlphaRuns::breakRun` | 🟢 | Line-by-line audited: two-phase run splitting logic (`x`, then `count`) matches Rust |
| `AlphaRuns::break_at` | `AlphaRuns::breakAt` | 🟢 | Line-by-line audited: same run-walk and split-at-x behavior used by clipping path |

### `third_party/tiny-skia/src/blend_mode.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `BlendMode::should_pre_scale_coverage` | `shouldPreScaleCoverage` | 🟢 | Line-by-line audited vs Rust `matches!` set; includes `Destination`, `DestinationOver`, `Plus`, `DestinationOut`, `SourceAtop`, `SourceOver`, `Xor` |
| `BlendMode::to_stage` | `toStage` | 🟢 | Line-by-line audited vs Rust `match`; all 29 blend-mode mappings + `Source -> nullopt` parity confirmed |

### `third_party/tiny-skia/src/edge_builder.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `ShiftedIntRect::new` | `ShiftedIntRect::create` | 🟡 | Shifted rect round-trip and overflow checks |
| `BasicEdgeBuilder::build_edges` | `BasicEdgeBuilder::buildEdges` | 🟡 | Build path output count, clip pathing, and edge-type coverage |
| `BasicEdgeBuilder::build` | `BasicEdgeBuilder::build` | 🟡 | Non-empty pathing and clip-path/early-failure checks |
| `combine_vertical` | `combineVertical` | 🟡 | Vertical adjacency equivalence checks |
| `edge_iter` | `pathIter` | 🟡 | Auto-close contour transitions |
| `PathEdgeIter::next` | `PathEdgeIter::next` | 🟡 | Move/Close and edge emission sequencing |
| `PathEdgeIter::close_line` | `PathEdgeIter::closeLine` | 🟡 | Auto-close closure to move point |

### `third_party/tiny-skia/src/edge_clipper.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `EdgeClipper::new` | `EdgeClipper::EdgeClipper` | 🟡 | Covered by `EdgeClipperTest` construction and iterator-path scenarios |
| `EdgeClipper::clip_line` | `EdgeClipper::clipLine` | 🟡 | Covered by `EdgeClipperTest.ClipLinePassesThroughWhenFullyInsideClip` and `ClipLineCanCullToRightOrPreserveWhenDisabled` |
| `EdgeClipper::push_line` | `ClippedEdges::pushLine` | 🟡 | Vertical and finite segment emission parity |
| `EdgeClipper::push_vline` | `EdgeClipper::pushVerticalLine` | 🟡 | Left/right boundary vertical clips |
| `EdgeClipper::clip_quad` | `EdgeClipper::clipQuad` | 🟡 | Covered by `EdgeClipperTest.ClipQuadFullyToLeftBecomesVerticalLine` and `ClipQuadFullyToRightProducesRightVerticalLineWhenCullingDisabled` |
| `EdgeClipper::clip_mono_quad` | `EdgeClipper::clipMonoQuad` | 🟡 | Left/right/inside clipping branches |
| `EdgeClipper::push_quad` | `EdgeClipper::pushQuad` | 🟡 | Reversed orientation emission |
| `EdgeClipper::clip_cubic` | `EdgeClipper::clipCubic` | 🟡 | Covered by `EdgeClipperTest.ClipCubicPartiallyInsideProducesCubicAndBoundaryLines` and `ClipVeryLargeCubicFallsBackToLineClipping` |
| `EdgeClipper::clip_mono_cubic` | `EdgeClipper::clipMonoCubic` | 🟡 | Left/right branch handling |
| `EdgeClipper::push_cubic` | `EdgeClipper::pushCubic` | 🟡 | Reversed orientation emission |
| `EdgeClipperIter::new` | `EdgeClipperIter::EdgeClipperIter` | 🟡 | Exercised via `EdgeClipperTest` iteration over clipped segment batches |
| `EdgeClipperIter::next` | `EdgeClipperIter::next` | 🟡 | Exercised via `EdgeClipperTest` sequence assertions across clip cases |
| `quick_reject` | `quickReject` | 🟡 | Indirectly covered by line/quad/cubic reject scenarios in `EdgeClipperTest` |
| `sort_increasing_y` | `sortIncreasingY` | 🟡 | Reversed source ordering |
| `chop_quad_in_y` | `chopQuadInY` | 🟡 | Indirectly covered via `EdgeClipperTest` quad clipping branches |
| `chop_mono_quad_at_x` | `path_geometry::chopMonoQuadAtX` | 🟡 | Intersection root validation |
| `chop_mono_quad_at_y` | `path_geometry::chopMonoQuadAtY` | 🟡 | Intersection root validation |
| `too_big_for_reliable_float_math` | `tooBigForReliableFloatMath` | 🟡 | Covered by `EdgeClipperTest.ClipVeryLargeCubicFallsBackToLineClipping` |
| `chop_cubic_in_y` | `chopCubicInY` | 🟡 | Top/bottom clipping and correction |
| `chop_mono_cubic_at_x` | `chopMonoCubicAtXFallback` | 🟡 | Exact/approximation fallback branch |
| `chop_mono_cubic_at_y` | `chopMonoCubicAtYFallback` | 🟡 | Exact/approximation fallback branch |
| `mono_cubic_closest_t` | `monoCubicClosestT` | 🟡 | Indirectly exercised through mono-cubic clipping paths in `EdgeClipperTest` |

### `third_party/tiny-skia/src/path.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `FillRule::Winding` | `FillRule::Winding` | 🟡 | Enum value mapping checks |
| `FillRule::EvenOdd` | `FillRule::EvenOdd` | 🟡 | Enum value mapping checks |
| `Path::bounds` | `Path::bounds` | 🟡 | Bounds from points with empty-path fallback |

### `third_party/tiny-skia/src/path_geometry.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `chop_quad_at` | `chopQuadAt` | 🟡 | Covered by `PathGeometryTest.ChopQuadAtInterpolatesPoints` |
| `chop_quad_at_x_extrema` | `chopQuadAtXExtrema` | 🟡 | Covered by `PathGeometryTest.ChopQuadAtXExtremaMonotonicLeavesInputIntact` |
| `chop_quad_at_y_extrema` | `chopQuadAtYExtrema` | 🟡 | Covered by `PathGeometryTest.ChopQuadAtYExtremaMonotonicLeavesInputIntact` and `ChopQuadAtYExtremaFlattensPeak` |
| `find_cubic_extrema` | `findCubicExtrema` | 🟡 | Correct t-values in simple and dual-extrema curves |
| `chop_cubic_at` | `chopCubicAt` | 🟡 | Covered by `PathGeometryTest.ChopCubicAtReturnsOriginalForEmptyTValues` and `ChopCubicAtSplitsAtOneCut` |
| `chop_cubic_at_x_extrema` | `chopCubicAtXExtrema` | 🟡 | X-monotone output flattening checks |
| `chop_cubic_at_y_extrema` | `chopCubicAtYExtrema` | 🟡 | Covered by `PathGeometryTest.ChopCubicAtYExtremaFlattensMonotonicYForCurve` |
| `chop_cubic_at_max_curvature` | `chopCubicAtMaxCurvature` | 🟡 | Covered by `PathGeometryTest.ChopCubicAtMaxCurvatureFiltersEndpointsAndSplits` and `ChopCubicAtMaxCurvatureNoInteriorRootsReturnsOriginalCurve` |
| `chop_mono_cubic_at_x` | `chopMonoCubicAtX` | 🟡 | Covered by `PathGeometryTest.ChopMonoCubicAtXFindsAndChopsAtIntercept` |
| `chop_mono_cubic_at_y` | `chopMonoCubicAtY` | 🟡 | Covered by `PathGeometryTest.ChopMonoCubicAtYReturnsFalseWithoutRoots` |
| `chop_mono_quad_at_x` | `chopMonoQuadAtX` | 🟡 | Covered by `PathGeometryTest.ChopMonoQuadAtXReturnsFalseWhenNoIntersection` |
| `chop_mono_quad_at_y` | `chopMonoQuadAtY` | 🟡 | Covered by `PathGeometryTest.ChopMonoQuadAtYReportsTAndLeavesBounds` |

### `third_party/tiny-skia/src/fixed_point.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `fdot6::from_i32` | `fdot6::fromI32` | 🟡 | Covered by `FixedPointFdot6Test.ConversionsAndRounding` |
| `fdot6::from_f32` | `fdot6::fromF32` | 🟡 | Covered by `FixedPointFdot6Test.ConversionsAndRounding` |
| `fdot6::floor` | `fdot6::floor` | 🟡 | Covered by `FixedPointFdot6Test.ConversionsAndRounding` |
| `fdot6::ceil` | `fdot6::ceil` | 🟡 | Covered by `FixedPointFdot6Test.ConversionsAndRounding` |
| `fdot6::round` | `fdot6::round` | 🟡 | Covered by `FixedPointFdot6Test.ConversionsAndRounding` |
| `fdot6::to_fdot16` | `fdot6::toFdot16` | 🟡 | Covered by `FixedPointFdot6And8Test.Fdot8ConversionAndBoundaries` |
| `fdot6::div` | `fdot6::div` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |
| `fdot6::can_convert_to_fdot16` | `fdot6::canConvertToFdot16` | 🟡 | Covered by `FixedPointFdot6And8Test.Fdot8ConversionAndBoundaries` |
| `fdot6::small_scale` | `fdot6::smallScale` | 🟡 | Covered by `FixedPointFdot6And8Test.Fdot8ConversionAndBoundaries` |
| `fdot8::from_fdot16` | `fdot8::fromFdot16` | 🟡 | Covered by `FixedPointFdot6And8Test.Fdot8ConversionAndBoundaries` |
| `fdot16::from_f32` | `fdot16::fromF32` | 🟡 | Covered by `FixedPointFdot16Test.FloatingConversionAndArithmetic` |
| `fdot16::floor_to_i32` | `fdot16::floorToI32` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |
| `fdot16::ceil_to_i32` | `fdot16::ceilToI32` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |
| `fdot16::round_to_i32` | `fdot16::roundToI32` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |
| `fdot16::mul` | `fdot16::mul` | 🟡 | Covered by `FixedPointFdot16Test.FloatingConversionAndArithmetic` |
| `fdot16::div` | `fdot16::divide` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |
| `fdot16::fast_div` | `fdot16::fastDiv` | 🟡 | Covered by `Fdot16AndFdot8.DivisionAndRoundingEdgeCases` |

### `third_party/tiny-skia/src/color.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `ColorU8::from_rgba` | `ColorU8::fromRgba` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` and `ColorFromRgbaEdgeCases` |
| `ColorU8::red` | `ColorU8::red` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `ColorU8::green` | `ColorU8::green` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `ColorU8::blue` | `ColorU8::blue` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `ColorU8::alpha` | `ColorU8::alpha` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `ColorU8::is_opaque` | `ColorU8::isOpaque` | 🟡 | Covered by `ColorTest.ColorFromRgbaEdgeCases` |
| `ColorU8::premultiply` | `ColorU8::premultiply` | 🟡 | Covered by `ColorTest.ColorUPremultiplyPreservesAlphaAndClamp` |
| `PremultipliedColorU8::from_rgba` | `PremultipliedColorU8::fromRgba` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` |
| `PremultipliedColorU8::from_rgba_unchecked` | `PremultipliedColorU8::fromRgbaUnchecked` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` unchecked path |
| `PremultipliedColorU8::red` | `PremultipliedColorU8::red` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` component checks |
| `PremultipliedColorU8::green` | `PremultipliedColorU8::green` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` component checks |
| `PremultipliedColorU8::blue` | `PremultipliedColorU8::blue` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` component checks |
| `PremultipliedColorU8::alpha` | `PremultipliedColorU8::alpha` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` component checks |
| `PremultipliedColorU8::is_opaque` | `PremultipliedColorU8::isOpaque` | 🟡 | Covered by `ColorTest.PremultipliedColorOptionValidation` |
| `PremultipliedColorU8::demultiply` | `PremultipliedColorU8::demultiply` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` |
| `Color::from_rgba_unchecked` | `Color::fromRgbaUnchecked` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` |
| `Color::from_rgba` | `Color::fromRgba` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` and `ColorFromRgbaEdgeCases` |
| `Color::from_rgba8` | `Color::fromRgba8` | 🟡 | Covered by `ColorTest.ColorConversionToU8AndPremultiplyRoundTrip` |
| `Color::red` | `Color::red` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `Color::green` | `Color::green` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `Color::blue` | `Color::blue` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `Color::alpha` | `Color::alpha` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` component checks |
| `Color::set_red` | `Color::setRed` | 🟡 | Covered by `ColorTest.ColorSettersClampToRange` |
| `Color::set_green` | `Color::setGreen` | 🟡 | Covered by `ColorTest.ColorSettersClampToRange` |
| `Color::set_blue` | `Color::setBlue` | 🟡 | Covered by `ColorTest.ColorSettersClampToRange` |
| `Color::set_alpha` | `Color::setAlpha` | 🟡 | Covered by `ColorTest.ColorSettersClampToRange` |
| `Color::apply_opacity` | `Color::applyOpacity` | 🟡 | Covered by `ColorTest.ColorFromRgbaAndOpacity` |
| `Color::is_opaque` | `Color::isOpaque` | 🟡 | Covered by `ColorTest.ColorFromRgbaEdgeCases` |
| `Color::premultiply` | `Color::premultiply` | 🟡 | Covered by `ColorTest.ColorConversionToU8AndPremultiplyRoundTrip` |
| `Color::to_color_u8` | `Color::toColorU8` | 🟡 | Covered by `ColorTest.ColorConversionToU8AndPremultiplyRoundTrip` |
| `PremultipliedColor::red` | `PremultipliedColor::red` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` component checks |
| `PremultipliedColor::green` | `PremultipliedColor::green` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` component checks |
| `PremultipliedColor::blue` | `PremultipliedColor::blue` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` component checks |
| `PremultipliedColor::alpha` | `PremultipliedColor::alpha` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` component checks |
| `PremultipliedColor::demultiply` | `PremultipliedColor::demultiply` | 🟡 | Covered by `ColorTest.PremultipliedColorDemultiply` |
| `PremultipliedColor::to_color_u8` | `PremultipliedColor::toColorU8` | 🟡 | Covered by `ColorTest.ColorConversionToU8AndPremultiplyRoundTrip` |
| `premultiply_u8` | `premultiplyU8` | 🟡 | Covered by `ColorTest.ColorUPremultiplyU8UsesFixedPointMultiply` |
| `color_f32_to_u8` | `colorF32ToU8` | 🟡 | Covered by `ColorTest.ColorConversionToU8AndPremultiplyRoundTrip` |
| `ColorSpace::expand_channel` | `expandChannel` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` |
| `ColorSpace::expand_color` | `expandColor` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` |
| `ColorSpace::compress_channel` | `compressChannel` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` |
| `ColorSpace::expand_stage` | `expandStage` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` stage mapping assertions |
| `ColorSpace::expand_dest_stage` | `expandDestStage` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` stage mapping assertions |
| `ColorSpace::compress_stage` | `compressStage` | 🟡 | Covered by `ColorTest.ColorSpaceTransforms` stage mapping assertions |
| `pipeline::Stage::*` | `tiny_skia::pipeline::Stage` | 🟡 | Covered by `ColorTest.PipelineStageOrderingMatchesRustReference` |

### `third_party/tiny-skia/src/math.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `bound` | `bound` | 🟢 | Line-by-line audited: Rust `max.min(value).max(min)` matches C++ clamp ordering exactly |
| `left_shift` | `leftShift` | 🟢 | Line-by-line audited: Rust `((value as u32) << shift) as i32` matches C++ unsigned-shift then cast |
| `left_shift64` | `leftShift64` | 🟢 | Line-by-line audited: Rust `((value as u64) << shift) as i64` matches C++ unsigned-shift then cast |
| `approx_powf` | `approxPowf` | 🟢 | Line-by-line audited: constants, bit-casts, floor/round branches, and infinity/zero guards match Rust formula |

### `third_party/tiny-skia/src/geom.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `ScreenIntRect::from_xywh` | `ScreenIntRect::fromXYWH` | 🟡 | Covered by `GeomTest.ScreenIntRectFromXYWHRejectsInvalidDimensions` and `ScreenIntRectFromXYWHRejectsOverflowAndBounds` |
| `ScreenIntRect::from_xywh_safe` | `ScreenIntRect::fromXYWHSafe` | 🟡 | Covered by `GeomTest.ScreenIntRectOperations` and constructor safety paths |
| `ScreenIntRect::x` | `ScreenIntRect::x` | 🟡 | Coordinate round-trip checks |
| `ScreenIntRect::y` | `ScreenIntRect::y` | 🟡 | Coordinate round-trip checks |
| `ScreenIntRect::width` | `ScreenIntRect::width` | 🟡 | Width read/write boundary tests |
| `ScreenIntRect::height` | `ScreenIntRect::height` | 🟡 | Height read/write boundary tests |
| `ScreenIntRect::width_safe` | `ScreenIntRect::widthSafe` | 🟡 | Width safety checks |
| `ScreenIntRect::left` | `ScreenIntRect::left` | 🟡 | Coordinate round-trip checks |
| `ScreenIntRect::top` | `ScreenIntRect::top` | 🟡 | Coordinate round-trip checks |
| `ScreenIntRect::right` | `ScreenIntRect::right` | 🟡 | Overflow guard checks |
| `ScreenIntRect::bottom` | `ScreenIntRect::bottom` | 🟡 | Overflow guard checks |
| `ScreenIntRect::size` | `ScreenIntRect::size` | 🟡 | Size extraction invariants |
| `ScreenIntRect::contains` | `ScreenIntRect::contains` | 🟡 | Containment property checks |
| `ScreenIntRect::to_int_rect` | `ScreenIntRect::toIntRect` | 🟡 | Struct conversion invariants |
| `ScreenIntRect::to_rect` | `ScreenIntRect::toRect` | 🟡 | Float conversion parity |
| `IntSizeExt::to_screen_int_rect` | `IntSize::toScreenIntRect` | 🟡 | Positioned rectangle smoke tests |
| `IntSize::from_wh` | `IntSize::fromWh` | 🟡 | Covered by `GeomTest.IntSizeFromWhRejectsZero` |
| `IntRect::from_xywh` | `IntRect::fromXYWH` | 🟡 | Covered by `GeomTest.IntRectFromXYWHRejectsInvalidInputs` |
| `IntRect::width` | `IntRect::width` | 🟡 | Width read/write checks |
| `IntRect::height` | `IntRect::height` | 🟡 | Height read/write checks |
| `IntRectExt::to_screen_int_rect` | `IntRect::toScreenIntRect` | 🟡 | Conversion validity checks |
| `Rect::from_ltrb` | `Rect::fromLtrb` | 🟡 | Covered by `GeomTest.RectFromLtrbRejectsInvalidBounds` |
| `int_rect_to_screen` | `intRectToScreen` | 🟡 | Cross-type conversion checks |

### `third_party/tiny-skia/src/blitter.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Mask::image` | `Mask::image` | 🟢 | Line-by-line audited: Rust `[u8;2] image` maps directly to C++ `std::array<uint8_t,2> image` |
| `Mask::bounds` | `Mask::bounds` | 🟢 | Line-by-line audited: Rust `ScreenIntRect bounds` maps directly to C++ `ScreenIntRect bounds` |
| `Mask::row_bytes` | `Mask::rowBytes` | 🟢 | Line-by-line audited: Rust `u32 row_bytes` maps to C++ `uint32_t rowBytes` (name-only casing change) |
| `Blitter::blit_h` | `Blitter::blitH` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_anti_h` | `Blitter::blitAntiH` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_v` | `Blitter::blitV` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_anti_h2` | `Blitter::blitAntiH2` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_anti_v2` | `Blitter::blitAntiV2` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_rect` | `Blitter::blitRect` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |
| `Blitter::blit_mask` | `Blitter::blitMask` | 🟢 | Line-by-line audited: default implementation is unreachable/abort in both Rust and C++ |

### `third_party/tiny-skia/src/edge.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Edge::as_line` | `Edge::asLine` | 🟢 | Line-by-line audited: enum/variant dispatch to embedded `LineEdge` matches Rust `match` branches |
| `Edge::as_line_mut` | `Edge::asLine` | 🟢 | Line-by-line audited: mutable delegate dispatch matches Rust `match` branches |
| `LineEdge::new` | `LineEdge::create` | 🟢 | Line-by-line audited: scale conversion, winding swap, zero-height reject, slope/dy setup, and field writes match Rust |
| `LineEdge::is_vertical` | `LineEdge::isVertical` | 🟢 | Line-by-line audited: `dx == 0` parity |
| `LineEdge::update` | `LineEdge::update` | 🟢 | Line-by-line audited: fixed-point downshift, zero-height reject, slope recompute, and edge state updates match Rust |
| `QuadraticEdge::new` | `QuadraticEdge::create` | 🟢 | Line-by-line audited: constructor delegates to internal setup + first `update()` gate, same as Rust |
| `QuadraticEdge::new2` | `QuadraticEdge::create` | 🟢 | Line-by-line audited via internal `makeQuadraticEdge`: coefficient/shift derivation and state initialization match Rust `new2` |
| `QuadraticEdge::update` | `QuadraticEdge::update` | 🟢 | Line-by-line audited: segment stepping loop, success break conditions, and persisted state updates match Rust |
| `CubicEdge::new` | `CubicEdge::create` | 🟢 | Line-by-line audited: constructor delegates to internal setup + first `update()` gate, same as Rust |
| `CubicEdge::new2` | `CubicEdge::create` | 🟢 | Line-by-line audited via internal `makeCubicEdge`: delta/shift math, coefficient setup, and initial state match Rust `new2` |
| `CubicEdge::update` | `CubicEdge::update` | 🟢 | Line-by-line audited: forward-difference stepping, `newY` monotonic clamp, and loop termination semantics match Rust |

### `third_party/tiny-skia/src/line_clipper.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `MAX_POINTS` | `kLineClipperMaxPoints` | 🟢 | Line-by-line audited: Rust `MAX_POINTS: usize = 4` matches C++ `kLineClipperMaxPoints = 4` |
| `clip` | `clip` | 🟡 | Audit found mismatch risk: Rust computes some intersections from original `src`, C++ currently uses mutated `tmp` in corresponding branches |
| `intersect` | `intersect` | 🟡 | Audit found mismatch risk: Rust uses `sect_with_vertical(src, ...)` while C++ uses `sectWithVertical(tmp, ...)` after clipping |
