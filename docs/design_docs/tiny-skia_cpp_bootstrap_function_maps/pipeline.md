# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

### `third_party/tiny-skia/src/pipeline/mod.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `Stage` variants | `pipeline::Stage` | 🟡 | Covered by `ColorTest.PipelineStageOrderingMatchesRustReference` |
| `STAGES_COUNT` | `pipeline::kStagesCount` | 🟡 | Covered by `ColorTest.RasterPipelineCompileRecordsStageCountForExecutionPlan` |
| `Context` | `pipeline::Context` | 🟡 | Covered by `ColorTest.PipelineContextDefaultsMatchExpectedMembers` |
| `AAMaskCtx::copy_at_xy` | `AAMaskCtx::copyAtXY` | 🟡 | Covered by `ColorTest.PipelineAAMaskCtxCopyAtXYReturnsExpectedPairs` |
| `MaskCtx` | `pipeline::MaskCtx` | 🟡 | Covered by `ColorTest.MaskCtxOffsetMatchesPackedCoordinateFormula` |
| `SamplerCtx` | `pipeline::SamplerCtx` | 🟡 | Covered by `ColorTest.PipelineContextDefaultsMatchExpectedMembers` |
| `UniformColorCtx::from (implicit)` | `pipeline::UniformColorCtx` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `GradientColor::new` | `GradientColor::newFromRGBA` | 🟡 | Covered by `ColorTest.GradientColorNewFromRGBARoundTripsRGBAValues` |
| `GradientCtx::push_const_color` | `GradientCtx::pushConstColor` | 🟡 | Covered by `ColorTest.PipelineGradientCtxPushConstColorAppendsBiasAndZeroFactor` |
| `RasterPipelineBuilder::new` | `RasterPipelineBuilder` ctor | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `RasterPipelineBuilder::set_force_hq_pipeline` | `setForceHqPipeline` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileForcesHighForUnsupportedStage` |
| `RasterPipelineBuilder::push` | `push` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderPushAppendsStage` |
| `RasterPipelineBuilder::push_transform` | `pushTransform` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderPushTransformSkipsIdentity` and `RasterPipelineBuilderPushTransformStoresNonIdentityTransform` |
| `RasterPipelineBuilder::push_uniform_color` | `pushUniformColor` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |
| `RasterPipelineBuilder::compile` | `compile` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileUsesLowpByDefaultForSupportedStages`, `RasterPipelineBuilderCompileForcesHighForUnsupportedStage`, and `RasterPipelineBuilderCompileStageOverflowSkipsBeyondCapacitySafely` |
| `RasterPipeline::run` | `RasterPipeline::run` | 🟡 | Dispatch path covered by `ColorTest.PipelineLowpStartUsesFullAndTailChunks` and `PipelineHighpStartUsesFullAndTailChunks`; full stage effects remain incomplete |
| `RasterPipeline` | `pipeline::RasterPipeline` | 🟡 | Covered by `ColorTest.RasterPipelineBuilderCompileBuildsExpectedKindAndContext` |

### `third_party/tiny-skia/src/pipeline/highp.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `STAGE_WIDTH` | `highp::kStageWidth` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `StageFn` | `highp::StageFn` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr_eq` | `highp::fnPtrEq` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr` | `highp::fnPtr` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `start` | `highp::start` | 🟡 | Covered by `ColorTest.PipelineHighpStartUsesFullAndTailChunks` |
| `just_return` | `highp::justReturn` | 🧩 | Placeholder no-op stage; full stage behavior not applicable here |
| `destination_atop` | `highp::destination_atop` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_in` | `highp::destination_in` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_out` | `highp::destination_out` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_over` | `highp::destination_over` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_atop` | `highp::source_atop` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_in` | `highp::source_in` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_out` | `highp::source_out` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_over` | `highp::source_over` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `clear` | `highp::clear` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `modulate` | `highp::modulate` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `multiply` | `highp::multiply` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `plus` | `highp::plus` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `screen` | `highp::screen` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `xor` | `highp::x_or` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `color_burn` | `highp::color_burn` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `color_dodge` | `highp::color_dodge` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `STAGES` | `highp::STAGES` | 🟡 | Covered by stage-table execution in `ColorTest.PipelineHighpStartUsesFullAndTailChunks` |

### `third_party/tiny-skia/src/pipeline/lowp.rs`
| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `STAGE_WIDTH` | `lowp::kStageWidth` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `StageFn` | `lowp::StageFn` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr_eq` | `lowp::fnPtrEq` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `fn_ptr` | `lowp::fnPtr` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `start` | `lowp::start` | 🟡 | Covered by `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |
| `just_return` | `lowp::justReturn` | 🧩 | Placeholder no-op stage; full stage behavior not applicable here |
| `destination_atop` | `lowp::destination_atop` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_in` | `lowp::destination_in` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_out` | `lowp::destination_out` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `destination_over` | `lowp::destination_over` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_atop` | `lowp::source_atop` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_in` | `lowp::source_in` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_out` | `lowp::source_out` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `source_over` | `lowp::source_over` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `clear` | `lowp::clear` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `modulate` | `lowp::modulate` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `multiply` | `lowp::multiply` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `plus` | `lowp::plus` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `screen` | `lowp::screen` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `xor` | `lowp::x_or` | 🟡 | Covered by `ColorTest.PipelineHighpLowpFunctionsHaveExpectedSignaturesAndDefaults` |
| `STAGES` | `lowp::STAGES` | 🟡 | Covered by stage-table execution in `ColorTest.PipelineLowpStartUsesFullAndTailChunks` |
