# Function Mapping Tables

Legend: `☐` Not started, `🧩` Stub only, `🟡` Implemented/tested (Rust completeness not yet vetted), `🟢` Rust-completeness vetted (line-by-line against Rust), `✅` Verified parity sign-off (user-requested audit complete), `⏸` Blocked.

## `third_party/tiny-skia/src/wide/mod.rs` -> `src/tiny_skia/wide/Mod.cpp` / `src/tiny_skia/wide/Mod.h`

| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `generic_bit_blend` | `tiny_skia::wide::genericBitBlend` | 🟡 | Bit-expression ported directly; covered by `WideModTest.GenericBitBlendMatchesRustFormula`. |
| `FasterMinMax::faster_min` | `tiny_skia::wide::fasterMin` | 🟡 | Branch ordering mirrors Rust trait impl; covered by `WideModTest.FasterMinReturnsSmallerOperand`. |
| `FasterMinMax::faster_max` | `tiny_skia::wide::fasterMax` | 🟡 | Branch ordering mirrors Rust trait impl; covered by `WideModTest.FasterMaxReturnsLargerOperand`. |

## `third_party/tiny-skia/src/wide/f32x4_t.rs` -> `src/tiny_skia/wide/F32x4T.cpp` / `src/tiny_skia/wide/F32x4T.h`

| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `f32x4::splat` | `tiny_skia::wide::F32x4T::splat` | 🟡 | Direct lane broadcast port; covered by min/max and blend tests that consume splat-like lane semantics. |
| `f32x4::abs` | `tiny_skia::wide::F32x4T::abs` | 🟡 | Scalar fallback semantics ported with per-lane absolute value; covered by `F32x4TTest.AbsClearsSignBitPerLane`. |
| `f32x4::max` | `tiny_skia::wide::F32x4T::max` | 🟡 | Scalar fallback path uses `fasterMax` per lane; covered by `F32x4TTest.MinAndMaxUsePerLaneComparisons`. |
| `f32x4::min` | `tiny_skia::wide::F32x4T::min` | 🟡 | Scalar fallback path uses `fasterMin` per lane; covered by `F32x4TTest.MinAndMaxUsePerLaneComparisons`. |
| `f32x4::{cmp_eq,cmp_ne,cmp_ge,cmp_gt,cmp_le,cmp_lt}` | `tiny_skia::wide::F32x4T::{cmpEq,cmpNe,cmpGe,cmpGt,cmpLe,cmpLt}` | 🟡 | Scalar mask encoding (`0xFFFFFFFF`/`0`) ported; `cmpEq` mask bits validated in `F32x4TTest.ComparisonsReturnAllOnesMaskOrZero`. |
| `f32x4::blend` | `tiny_skia::wide::F32x4T::blend` | 🟡 | Scalar fallback delegates to bit-blend equivalent via `genericBitBlend`; covered by `F32x4TTest.BlendSelectsTrueAndFalseLanesByMaskBits`. |

## `third_party/tiny-skia/src/wide/i32x4_t.rs` -> `src/tiny_skia/wide/I32x4T.cpp` / `src/tiny_skia/wide/I32x4T.h`

| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `i32x4::splat` | `tiny_skia::wide::I32x4T::splat` | 🟡 | Direct lane broadcast constructor port. |
| `i32x4::blend` | `tiny_skia::wide::I32x4T::blend` | 🟡 | Bit-select path via `genericBitBlend`; covered by `I32x4TTest.BlendUsesMaskBitsPerLane`. |
| `i32x4::{cmp_eq,cmp_gt,cmp_lt}` | `tiny_skia::wide::I32x4T::{cmpEq,cmpGt,cmpLt}` | 🟡 | Scalar fallback mask encoding (`-1` / `0`) ported; covered by `I32x4TTest.ComparisonsEmitMinusOneMasks`. |
| `i32x4::{to_f32x4,to_f32x4_bitcast}` | `tiny_skia::wide::I32x4T::{toF32x4,toF32x4Bitcast}` | 🟡 | Numeric cast and bitcast conversion paths ported; covered by `I32x4TTest.ToF32x4AndBitcastMatchRustConversions`. |
| `impl Add for i32x4` | `tiny_skia::wide::I32x4T::operator+` | 🟡 | Wrapping lane-add semantics via unsigned bit math; covered by `I32x4TTest.AddAndMulUseWrappingSemantics`. |
| `impl Mul for i32x4` | `tiny_skia::wide::I32x4T::operator*` | 🟡 | Wrapping lane-mul semantics via unsigned bit math; covered by `I32x4TTest.AddAndMulUseWrappingSemantics`. |
| `impl {BitAnd,BitOr,BitXor} for i32x4` | `tiny_skia::wide::I32x4T::{operator&,operator|,operator^}` | 🟡 | Per-lane bitwise ops ported; behavior exercised through blend/mask pathways and direct operator implementation. |

## `third_party/tiny-skia/src/wide/u32x4_t.rs` -> `src/tiny_skia/wide/U32x4T.cpp` / `src/tiny_skia/wide/U32x4T.h`

| Rust function/item | C++ function/item | Status | Evidence / Notes |
| --- | --- | --- | --- |
| `u32x4::splat` | `tiny_skia::wide::U32x4T::splat` | 🟡 | Direct lane broadcast constructor port. |
| `u32x4::cmp_eq` | `tiny_skia::wide::U32x4T::cmpEq` | 🟡 | Scalar fallback equality mask (`u32::MAX`/`0`) ported; covered by `U32x4TTest.CmpEqProducesAllOnesMaskPerLane`. |
| `u32x4::shl` | `tiny_skia::wide::U32x4T::shl<Rhs>` | 🟡 | Template shift-left mirrors per-lane scalar fallback; covered by `U32x4TTest.ShiftOperationsMatchRustScalarFallback`. |
| `u32x4::shr` | `tiny_skia::wide::U32x4T::shr<Rhs>` | 🟡 | Template shift-right mirrors per-lane scalar fallback; covered by `U32x4TTest.ShiftOperationsMatchRustScalarFallback`. |
| `impl Not for u32x4` | `tiny_skia::wide::U32x4T::operator~` | 🟡 | Per-lane bitwise NOT matches Rust fallback; covered by `U32x4TTest.BitwiseAndAddOpsArePerLane`. |
| `impl Add for u32x4` | `tiny_skia::wide::U32x4T::operator+` | 🟡 | Unsigned wrap add matches Rust scalar fallback; covered by `U32x4TTest.BitwiseAndAddOpsArePerLane`. |
| `impl {BitAnd,BitOr} for u32x4` | `tiny_skia::wide::U32x4T::{operator&,operator|}` | 🟡 | Per-lane bitwise ops ported; covered by `U32x4TTest.BitwiseAndAddOpsArePerLane`. |

