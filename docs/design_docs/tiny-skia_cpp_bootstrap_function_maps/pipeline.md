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
| `STAGES` | `lowp::STAGES` | 🟢 | Line-by-line audited: Covered by stage-table execution in `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |
