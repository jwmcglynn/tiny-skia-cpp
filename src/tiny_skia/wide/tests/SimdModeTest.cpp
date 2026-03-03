#include "tiny_skia/wide/Mod.h"

#include "gtest/gtest.h"

namespace {

TEST(SimdModeTest, CompileTimeAndRuntimeModeMatch) {
#if defined(TINYSKIA_CFG_IF_SIMD_NATIVE)
  EXPECT_EQ(tiny_skia::wide::configuredSimdBuildMode(),
            tiny_skia::wide::SimdBuildMode::kNative);
  EXPECT_STREQ(tiny_skia::wide::configuredSimdBuildModeName(), "native");
#elif defined(TINYSKIA_CFG_IF_SIMD_SCALAR)
  EXPECT_EQ(tiny_skia::wide::configuredSimdBuildMode(),
            tiny_skia::wide::SimdBuildMode::kScalar);
  EXPECT_STREQ(tiny_skia::wide::configuredSimdBuildModeName(), "scalar");
#else
  GTEST_FAIL() << "SIMD mode define missing";
#endif
}

TEST(SimdModeTest, SelectedBackendMatchesModePolicy) {
#if defined(TINYSKIA_CFG_IF_SIMD_SCALAR)
  EXPECT_EQ(tiny_skia::wide::configuredSimdBackend(),
            tiny_skia::wide::SimdBackend::kScalar);
  EXPECT_STREQ(tiny_skia::wide::configuredSimdBackendName(), "scalar");
#else
  // Native mode selects the best available backend for the compile target.
  EXPECT_STREQ(tiny_skia::wide::configuredSimdBackendName(),
               tiny_skia::wide::backend::backendName(
                   tiny_skia::wide::backend::detectNativeBackend()));
#endif
}

}  // namespace
