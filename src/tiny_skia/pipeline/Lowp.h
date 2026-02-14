#pragma once

#include <array>
#include <cstddef>

#include "tiny_skia/pipeline/Mod.h"

namespace tiny_skia::pipeline::lowp {

struct Pipeline;

using StageFn = void (*)(Pipeline&);

constexpr std::size_t kStageWidth = 16;

void justReturn(Pipeline&);

void start(const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& functions,
           const std::array<StageFn, tiny_skia::pipeline::kMaxStages>& tail_functions,
           const ScreenIntRect& rect,
           const AAMaskCtx& aa_mask_ctx,
           const MaskCtx& mask_ctx,
           Context& ctx,
           SubPixmapMut* pixmap_dst);

bool fnPtrEq(StageFn a, StageFn b);
const void* fnPtr(StageFn fn);

extern const std::array<StageFn, kStagesCount> STAGES;

}  // namespace tiny_skia::pipeline::lowp
