#include <gtest/gtest.h>

#include "tiny_skia/Lib.h"

TEST(LibTest, LibraryVersionMatchesExpectedConstant) {
  EXPECT_EQ(tiny_skia::libraryVersion(), tiny_skia::kLibraryVersion);
  EXPECT_EQ(tiny_skia::kLibraryVersion, 1);
}
