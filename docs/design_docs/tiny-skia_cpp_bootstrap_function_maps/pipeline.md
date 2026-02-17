# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/pipeline/mod.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Stage` variants | `pipeline::Stage` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineStageOrderingMatchesRustReference` |
| `STAGES_COUNT` | `pipeline::kStagesCount` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineCompileRecordsStageCountForExecutionPlan` |
| `Context` | `pipeline::Context` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineContextDefaultsMatchExpectedMembers` |
| `AAMaskCtx::copy_at_xy` | `AAMaskCtx::copyAtXY` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineAAMaskCtxCopyAtXYReturnsExpectedPairs` |
| `MaskCtx` | `pipeline::MaskCtx` | 🟢 | Line-by-line audited: Covered by `ColorTest.MaskCtxOffsetMatchesPackedCoordinateFormula` |
| `SamplerCtx` | `pipeline::SamplerCtx` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineContextDefaultsMatchExpectedMembers` |
| `UniformColorCtx::from (implicit)` | `pipeline::UniformColorCtx` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `GradientColor::new` | `GradientColor::newFromRGBA` | 🟢 | Line-by-line audited: Covered by `ColorTest.GradientColorNewFromRGBARoundTripsRGBAValues` |
| `GradientCtx::push_const_color` | `GradientCtx::pushConstColor` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineGradientCtxPushConstColorAppendsBiasAndZeroFactor` |
| `RasterPipelineBuilder::new` | `RasterPipelineBuilder` ctor | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `RasterPipelineBuilder::set_force_hq_pipeline` | `setForceHqPipeline` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileForcesHighForUnsupportedStage` |
| `RasterPipelineBuilder::push` | `push` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderPushAppendsStage` |
| `RasterPipelineBuilder::push_transform` | `pushTransform` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderPushTransformSkipsIdentity` and `RasterPipelineBuilderPushTransformStoresNonIdentityTransform` |
| `RasterPipelineBuilder::push_uniform_color` | `pushUniformColor` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `RasterPipelineBuilder::compile` | `compile` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileUsesLowpByDefaultForSupportedStages`, `RasterPipelineBuilderCompileForcesHighForUnsupportedStage`, and `RasterPipelineBuilderCompileStageOverflowSkipsBeyondCapacitySafely` |
| `RasterPipeline::run` | `RasterPipeline::run` | 🟢 | Line-by-line audited: kind-dispatch, highp/lowp start call wiring, mutable context handoff, and lowp no-`pixmap_src` behavior match Rust |
| `RasterPipeline` | `pipeline::RasterPipeline` | 🟢 | Line-by-line audited: Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |

### `third_party/tiny-skia/src/pipeline/highp.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `STAGE_WIDTH` | `highp::kStageWidth` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `StageFn` | `highp::StageFn` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr_eq` | `highp::fnPtrEq` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr` | `highp::fnPtr` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `start` | `highp::start` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpStartUsesFullAndTailChunks` |
| `just_return` | `highp::justReturn` | 🟢 | Line-by-line audited: terminal no-op stage parity (`just_return` intentionally ends stage chain) |
| `destination_atop` | `highp::destination_atop` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_in` | `highp::destination_in` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_out` | `highp::destination_out` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_over` | `highp::destination_over` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_atop` | `highp::source_atop` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_in` | `highp::source_in` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_out` | `highp::source_out` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_over` | `highp::source_over` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `clear` | `highp::clear` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `modulate` | `highp::modulate` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `multiply` | `highp::multiply` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `plus` | `highp::plus` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `screen` | `highp::screen` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `xor` | `highp::x_or` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `color_burn` | `highp::color_burn` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `color_dodge` | `highp::color_dodge` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `load_dst` | `highp::load_dst` | 🟡 | Pixel I/O: loads RGBA destination via load_8888 helper (byte→float [0,1]). Exercised through blitter pipeline tests. |
| `store` | `highp::store` | 🟡 | Pixel I/O: stores RGBA via store_8888 helper (float [0,1]→byte with lround). Exercised through blitter pipeline tests. |
| `mask_u8` | `highp::mask_u8` | 🟡 | Reads external mask coverage, scales source channels. Early-returns if all-zero. Exercised through blitter pipeline tests. |
| `source_over_rgba` | `highp::source_over_rgba` | 🟡 | Combined load+SourceOver+store in one stage. Exercised through blitter pipeline tests. |
| `gather` | `highp::gather` | 🟡 | Texture sampling via SamplerCtx with tiling modes. Covered by PipelineStagesTest.PatternFill*. |
| `transform` | `highp::transform` | 🟡 | Affine transform via TransformCtx. Covered by PipelineStagesTest.LinearGradient*, RadialGradient*, PatternFill*. |
| `reflect` | `highp::reflect` | 🟡 | Reflect tiling for texture coordinates. Covered by pattern/gradient tests. |
| `repeat` | `highp::repeat` | 🟡 | Repeat tiling for texture coordinates. Covered by pattern/gradient tests. |
| `bilinear` | `highp::bilinear` | 🟡 | Bilinear interpolation via SamplerCtx. Covered by PipelineStagesTest.PatternFillBilinear. |
| `bicubic` | `highp::bicubic` | 🟡 | Bicubic interpolation via SamplerCtx (Mitchell-Netravali). Covered by PipelineStagesTest.PatternFillBicubic. |
| `pad_x1` | `highp::pad_x1` | 🟡 | Clamp gradient t to [0,1]. Covered by PipelineStagesTest.LinearGradient2Stop. |
| `reflect_x1` | `highp::reflect_x1` | 🟡 | Reflect gradient t in [0,1]. Covered by PipelineStagesTest.ReflectGradient. |
| `repeat_x1` | `highp::repeat_x1` | 🟡 | Repeat gradient t in [0,1]. Covered by PipelineStagesTest.RepeatGradient. |
| `gradient` | `highp::gradient` | 🟡 | Multi-stop gradient interpolation via GradientCtx. Covered by PipelineStagesTest.LinearGradient3Stop. |
| `evenly_spaced_2_stop_gradient` | `highp::evenly_spaced_2_stop_gradient` | 🟡 | 2-stop gradient fast path. Covered by PipelineStagesTest.LinearGradient2Stop. |
| `xy_to_unit_angle` | `highp::xy_to_unit_angle` | 🟡 | atan2 approximation for sweep gradients. Covered by PipelineStagesTest.SweepGradient*. |
| `xy_to_radius` | `highp::xy_to_radius` | 🟡 | Euclidean distance for radial gradients. Covered by PipelineStagesTest.RadialGradient*. |
| `xy_to_2pt_conical_focal_on_circle` | `highp::xy_to_2pt_conical_focal_on_circle` | 🟡 | 2pt-conical focal-on-circle mapping. Exercised through painter pipeline. |
| `xy_to_2pt_conical_well_behaved` | `highp::xy_to_2pt_conical_well_behaved` | 🟡 | 2pt-conical well-behaved mapping. Exercised through painter pipeline. |
| `xy_to_2pt_conical_smaller` | `highp::xy_to_2pt_conical_smaller` | 🟡 | 2pt-conical smaller-radius mapping. Exercised through painter pipeline. |
| `xy_to_2pt_conical_greater` | `highp::xy_to_2pt_conical_greater` | 🟡 | 2pt-conical greater-radius mapping. Exercised through painter pipeline. |
| `xy_to_2pt_conical_strip` | `highp::xy_to_2pt_conical_strip` | 🟡 | 2pt-conical strip mapping. Exercised through painter pipeline. |
| `mask_2pt_conical_nan` | `highp::mask_2pt_conical_nan` | 🟡 | NaN masking for 2pt-conical. Exercised through painter pipeline. |
| `mask_2pt_conical_degenerates` | `highp::mask_2pt_conical_degenerates` | 🟡 | Degenerate masking for 2pt-conical. Exercised through painter pipeline. |
| `apply_vector_mask` | `highp::apply_vector_mask` | 🟡 | Applies float mask to source channels. Exercised through painter pipeline. |
| `alter_2pt_conical_compensate_focal` | `highp::alter_2pt_conical_compensate_focal` | 🟡 | Focal compensation for 2pt-conical. Exercised through painter pipeline. |
| `alter_2pt_conical_unswap` | `highp::alter_2pt_conical_unswap` | 🟡 | Unswap transform for 2pt-conical. Exercised through painter pipeline. |
| `negate_x` | `highp::negate_x` | 🟡 | Negates x coordinate. Exercised through painter pipeline. |
| `apply_concentric_scale_bias` | `highp::apply_concentric_scale_bias` | 🟡 | Scale+bias for concentric gradients. Exercised through painter pipeline. |
| `gamma_expand_2` | `highp::gamma_expand_2` | 🟡 | Gamma 2.0 expand for source. Exercised through painter pipeline. |
| `gamma_expand_dst_2` | `highp::gamma_expand_dst_2` | 🟡 | Gamma 2.0 expand for destination. Exercised through painter pipeline. |
| `gamma_compress_2` | `highp::gamma_compress_2` | 🟡 | Gamma 2.0 compress. Exercised through painter pipeline. |
| `gamma_expand_22` | `highp::gamma_expand_22` | 🟡 | Gamma 2.2 expand for source. Exercised through painter pipeline. |
| `gamma_expand_dst_22` | `highp::gamma_expand_dst_22` | 🟡 | Gamma 2.2 expand for destination. Exercised through painter pipeline. |
| `gamma_compress_22` | `highp::gamma_compress_22` | 🟡 | Gamma 2.2 compress. Exercised through painter pipeline. |
| `gamma_expand_srgb` | `highp::gamma_expand_srgb` | 🟡 | sRGB gamma expand for source. Exercised through painter pipeline. |
| `gamma_expand_dst_srgb` | `highp::gamma_expand_dst_srgb` | 🟡 | sRGB gamma expand for destination. Exercised through painter pipeline. |
| `gamma_compress_srgb` | `highp::gamma_compress_srgb` | 🟡 | sRGB gamma compress. Exercised through painter pipeline. |
| `darken` | `highp::darken` | 🟡 | Darken blend mode. Covered by PipelineStagesTest.DarkenBlend. |
| `lighten` | `highp::lighten` | 🟡 | Lighten blend mode. Covered by PipelineStagesTest.LightenBlend. |
| `difference` | `highp::difference` | 🟡 | Difference blend mode. Covered by PipelineStagesTest.DifferenceBlend. |
| `exclusion` | `highp::exclusion` | 🟡 | Exclusion blend mode. Covered by PipelineStagesTest.ExclusionBlend. |
| `hard_light` | `highp::hard_light` | 🟡 | Hard light blend mode. Covered by PipelineStagesTest.HardLightBlend. |
| `overlay` | `highp::overlay` | 🟡 | Overlay blend mode. Covered by PipelineStagesTest.OverlayBlend. |
| `soft_light` | `highp::soft_light` | 🟡 | Soft light blend mode. Covered by PipelineStagesTest.SoftLightBlend. |
| `hue` | `highp::hue` | 🟡 | Hue blend mode (non-separable). Covered by PipelineStagesTest.HueBlend. |
| `saturation` | `highp::saturation` | 🟡 | Saturation blend mode (non-separable). Covered by PipelineStagesTest.SaturationBlend. |
| `color` | `highp::color` | 🟡 | Color blend mode (non-separable). Covered by PipelineStagesTest.ColorBlend. |
| `luminosity` | `highp::luminosity` | 🟡 | Luminosity blend mode (non-separable). Covered by PipelineStagesTest.LuminosityBlend. |
| `STAGES` | `highp::STAGES` | 🟢 | Line-by-line audited: Covered by stage-table execution in `ColorTest.PipelineHighpStartUsesFullAndTailChunks` |

### `third_party/tiny-skia/src/pipeline/lowp.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `STAGE_WIDTH` | `lowp::kStageWidth` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `StageFn` | `lowp::StageFn` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr_eq` | `lowp::fnPtrEq` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr` | `lowp::fnPtr` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `start` | `lowp::start` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |
| `just_return` | `lowp::justReturn` | 🟢 | Line-by-line audited: terminal no-op stage parity (`just_return` intentionally ends stage chain) |
| `destination_atop` | `lowp::destination_atop` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_in` | `lowp::destination_in` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_out` | `lowp::destination_out` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_over` | `lowp::destination_over` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_atop` | `lowp::source_atop` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_in` | `lowp::source_in` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_out` | `lowp::source_out` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_over` | `lowp::source_over` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `clear` | `lowp::clear` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `modulate` | `lowp::modulate` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `multiply` | `lowp::multiply` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `plus` | `lowp::plus` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `screen` | `lowp::screen` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `xor` | `lowp::x_or` | 🟢 | Line-by-line audited: Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `load_dst` | `lowp::load_dst` | 🟡 | Pixel I/O: loads RGBA destination via load_8888_lowp (byte→float [0,255]). Exercised through blitter pipeline tests. |
| `store` | `lowp::store` | 🟡 | Pixel I/O: stores RGBA via store_8888_lowp (float [0,255]→byte truncation). Exercised through blitter pipeline tests. |
| `load_dst_u8` | `lowp::load_dst_u8` | 🟡 | Loads single-byte mask destination into da. Exercised through createMask blitter tests. |
| `store_u8` | `lowp::store_u8` | 🟡 | Stores alpha channel as single byte. Exercised through createMask blitter tests. |
| `load_mask_u8` | `lowp::load_mask_u8` | 🟡 | Reads mask coverage into source alpha, zeros RGB. Exercised through blitter pipeline tests. |
| `mask_u8` | `lowp::mask_u8` | 🟡 | Reads external mask coverage, scales source channels with lowp div255. Early-returns if all-zero. Exercised through blitter pipeline tests. |
| `source_over_rgba` | `lowp::source_over_rgba` | 🟡 | Combined load+SourceOver+store in lowp [0,255] range. Exercised through blitter pipeline tests. |
| `darken` | `lowp::darken` | 🟡 | Darken blend in [0,255] range with div255 approximation. Covered by PipelineStagesTest.DarkenBlend. |
| `lighten` | `lowp::lighten` | 🟡 | Lighten blend in [0,255] range with div255 approximation. Covered by PipelineStagesTest.LightenBlend (±2 tolerance). |
| `difference` | `lowp::difference` | 🟡 | Difference blend in [0,255] range. Covered by PipelineStagesTest.DifferenceBlend (±3 tolerance for div255 bias). |
| `exclusion` | `lowp::exclusion` | 🟡 | Exclusion blend in [0,255] range. Covered by PipelineStagesTest.ExclusionBlend (±2 tolerance). |
| `hard_light` | `lowp::hard_light` | 🟡 | Hard light blend in [0,255] range. Covered by PipelineStagesTest.HardLightBlend. |
| `overlay` | `lowp::overlay` | 🟡 | Overlay blend in [0,255] range. Covered by PipelineStagesTest.OverlayBlend. |
| `transform` | `lowp::transform` | 🟡 | Affine transform via TransformCtx. Covered by lowp gradient pipeline tests. |
| `pad_x1` | `lowp::pad_x1` | 🟡 | Clamp gradient t to [0,1]. Covered by lowp gradient pipeline tests. |
| `reflect_x1` | `lowp::reflect_x1` | 🟡 | Reflect gradient t in [0,1]. Covered by PipelineStagesTest.ReflectGradient. |
| `repeat_x1` | `lowp::repeat_x1` | 🟡 | Repeat gradient t in [0,1]. Covered by PipelineStagesTest.RepeatGradient. |
| `gradient` | `lowp::gradient` | 🟡 | Multi-stop gradient with [0,1]→[0,255] conversion. Covered by PipelineStagesTest.LinearGradient3Stop. |
| `evenly_spaced_2_stop_gradient` | `lowp::evenly_spaced_2_stop_gradient` | 🟡 | 2-stop gradient with [0,1]→[0,255] conversion. Covered by PipelineStagesTest.LinearGradient2Stop. |
| `xy_to_radius` | `lowp::xy_to_radius` | 🟡 | Euclidean distance for radial gradients. Covered by PipelineStagesTest.RadialGradient*. |
| `STAGES` | `lowp::STAGES` | 🟢 | Line-by-line audited: Fixed missing Luminosity entry. Covered by stage-table execution in `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |


### `third_party/tiny-skia/src/pipeline/blitter.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `RasterPipelineBlitter` | `pipeline::RasterPipelineBlitter` | 🟡 | Pipeline-driven blitter with 3 compiled RasterPipelines (blit_anti_h_rp, blit_rect_rp, blit_mask_rp). Holds optional memset_color, external mask, and pixmap_src_storage matching Rust struct layout. |
| `RasterPipelineBlitter::new` | `RasterPipelineBlitter::create` | 🟡 | Builds 3 pipelines with blend mode selection (SourceOver→Source strength reduction, Clear→Source, memset optimization). Takes BlendMode parameter (default SourceOver). Covered by `PipelineBlitterTest.CreateRejectsNullPixmap`, `CreateRejectsMaskSizeMismatch`, `CreateWithExternalMaskModulatesRectAlpha`, and all `Create*` blitting tests. |
| `RasterPipelineBlitter::new_mask` | `RasterPipelineBlitter::createMask` | 🟡 | Builds 3 pipelines for mask mode (UniformColor(white) + Lerp1Float/LerpU8 + Store). Covered by `PipelineBlitterTest.CreateMaskRejectsNullPixmap` and all mask-mode blitting tests. |
| `Blitter::blit_h` | `RasterPipelineBlitter::blitH` | 🟡 | Delegates to blitRect; matches Rust. |
| `Blitter::blit_anti_h` | `RasterPipelineBlitter::blitAntiH` | 🟡 | Run-walk with transparent/partial/opaque coverage branches. Partial coverage uses blit_anti_h_rp_ with Scale1Float. Covered by `PipelineBlitterTest.BlitAntiHRespectsRunsAndCoverageKinds` and `BlitAntiHStopsAtRunSentinel`. |
| `Blitter::blit_v` | `RasterPipelineBlitter::blitV` | 🟡 | Uses blit_mask_rp_ with AAMaskCtx (row_bytes=0 for uniform coverage). Covered by `PipelineBlitterTest.BlitVHandlesTransparentPartialAndOpaqueCoverage`, `PartialCoverageComposesWithExistingDestinationAlpha`, `CreateBlitVPartialCoverageSetsColorAndComposesAlpha`, and `CreateBlitVOpaqueCoverageComposesAcrossPasses`. |
| `Blitter::blit_anti_h2` | `RasterPipelineBlitter::blitAntiH2` | 🟡 | Uses blit_mask_rp_ with AAMaskCtx for 2-pixel horizontal coverage. Covered by `PipelineBlitterTest.BlitAntiH2WritesPerPixelCoverage`, `CreateBlitAntiH2PartialCoverageSetsColorAndAlpha`, and `CreateBlitAntiH2OpaqueCoverageComposesAcrossPasses`. |
| `Blitter::blit_anti_v2` | `RasterPipelineBlitter::blitAntiV2` | 🟡 | Uses blit_mask_rp_ with AAMaskCtx for 2-pixel vertical coverage. Covered by `PipelineBlitterTest.BlitAntiV2WritesSeparatePixelCoverages` and `CreateBlitAntiV2OpaqueCoverageComposesAcrossPasses`. |
| `Blitter::blit_rect` | `RasterPipelineBlitter::blitRect` | 🟡 | memset fast path for Source mode + pipeline fallback. Covered by `PipelineBlitterTest.CreateMaskAndBlitRectWritesOpaqueAlphaInRegion`, `BlitRectClipsToPixmapBounds`, and `CreateWithExternalMaskModulatesRectAlpha`. |
| `Blitter::blit_mask` | `RasterPipelineBlitter::blitMask` | 🟡 | Iterates mask data 2 pixels at a time through blit_mask_rp_ via AAMaskCtx. Covered by `PipelineBlitterTest.BlitMaskLerpsDestinationAlphaWithCoverageMap`, `BlitMaskClipsWhenClipExceedsMaskDimensions`, `BlitMaskPartialCoverageComposesAcrossPasses`, `CreateBlitMaskWritesColorWithMaskCoverage`, `CreateBlitMaskCombinesWithExternalMaskCoverage`, and `CreateBlitMaskPartialCoverageComposesAcrossPasses`. |

