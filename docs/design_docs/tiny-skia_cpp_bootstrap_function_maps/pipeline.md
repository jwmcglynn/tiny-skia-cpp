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
| `STAGES` | `lowp::STAGES` | 🟢 | Line-by-line audited: Covered by stage-table execution in `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |


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

