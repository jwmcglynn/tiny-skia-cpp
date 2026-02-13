#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>

#include "tiny_skia/AlphaRuns.h"
#include "tiny_skia/Math.h"

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    std::exit(1);
  }
}

void requireEq(int lhs, int rhs, const char* message) {
  if (lhs != rhs) {
    std::cerr << "FAIL: " << message << " (lhs=" << lhs << ", rhs=" << rhs << ")\n";
    std::exit(1);
  }
}

void requireEq(std::uint8_t lhs, std::uint8_t rhs, const char* message) {
  if (lhs != rhs) {
    std::cerr << "FAIL: " << message << " (lhs=" << static_cast<int>(lhs)
              << ", rhs=" << static_cast<int>(rhs) << ")\n";
    std::exit(1);
  }
}

void requireEq(std::int64_t lhs, std::int64_t rhs, const char* message) {
  if (lhs != rhs) {
    std::cerr << "FAIL: " << message << " (lhs=" << lhs << ", rhs=" << rhs << ")\n";
    std::exit(1);
  }
}

void requireApprox(float lhs, float rhs, float eps, const char* message) {
  if (std::fabs(lhs - rhs) > eps) {
    std::cerr << "FAIL: " << message << " (lhs=" << lhs << ", rhs=" << rhs
              << ", eps=" << eps << ")\n";
    std::exit(1);
  }
}

}  // namespace

int main() {
  requireEq(tiny_skia::kLengthU32One, 1, "LengthU32 one");

  requireEq(tiny_skia::bound(0, 3, 10), 3, "bound middle");
  requireEq(tiny_skia::bound(5, 3, 10), 5, "bound below");
  requireEq(tiny_skia::bound(0, 13, 10), 10, "bound above");

  requireEq(tiny_skia::leftShift(3, 4), 48, "leftShift");
  requireEq(tiny_skia::leftShift64(-1, 1), static_cast<std::int64_t>(0xFFFFFFFFFFFFFFFEULL),
            "leftShift64");

  requireApprox(tiny_skia::approxPowf(0.0f, 3.0f), 0.0f, 0.0f, "pow zero");
  requireApprox(tiny_skia::approxPowf(1.0f, 3.0f), 1.0f, 0.0f, "pow one");
  requireApprox(tiny_skia::approxPowf(2.0f, 1.0f), 2.0f, 0.0001f, "pow identity y=1");
  requireApprox(tiny_skia::approxPowf(4.0f, 1.0f), 4.0f, 0.0001f, "pow identity y=1 large");

  tiny_skia::AlphaRuns runs(6);
  require(runs.isEmpty(), "alpha runs initially empty");

  const auto midOffset = runs.add(0, 0, 2, 0, 50, 0);
  requireEq(midOffset, 2, "alpha add middle offset");
  require(!runs.isEmpty(), "alpha runs non-empty after add");
  requireEq(runs.alpha[0], static_cast<std::uint8_t>(50), "alpha runs max value applied");

  const auto stopOffset = runs.add(2, 0, 0, 8, 255, 0);
  requireEq(stopOffset, 2, "alpha add stop offset");
  requireEq(runs.alpha[2], static_cast<std::uint8_t>(8), "alpha stop contribution");

  std::array<tiny_skia::AlphaRun, 6> runData{};
  std::array<std::uint8_t, 6> alphaData{};
  runData[0] = tiny_skia::AlphaRun{5};
  tiny_skia::AlphaRuns::breakRun(std::span{runData}, std::span{alphaData}, 2, 2);
  require(runData[0].has_value(), "breakRun keeps first run");
  requireEq(runData[0].value(), 2, "breakRun first segment");
  require(runData[2].has_value(), "breakRun keeps second run");
  requireEq(runData[2].value(), 2, "breakRun second segment");
  require(runData[4].has_value(), "breakRun keeps third run");
  requireEq(runData[4].value(), 1, "breakRun third segment");

  std::array<tiny_skia::AlphaRun, 6> breakAtRuns{};
  std::array<std::uint8_t, 6> breakAtAlpha{};
  breakAtRuns[0] = tiny_skia::AlphaRun{5};
  breakAtAlpha[0] = 11;
  tiny_skia::AlphaRuns::breakAt(std::span{breakAtAlpha}, std::span{breakAtRuns}, 2);
  require(breakAtRuns[0].has_value(), "breakAt keeps first run");
  requireEq(breakAtRuns[0].value(), 2, "breakAt first segment");
  require(breakAtRuns[2].has_value(), "breakAt second run");
  requireEq(breakAtRuns[2].value(), 3, "breakAt second segment");
  requireEq(breakAtAlpha[2], breakAtAlpha[0], "breakAt alpha split");

  return 0;
}
