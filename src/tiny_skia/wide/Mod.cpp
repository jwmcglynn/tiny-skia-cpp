#include "tiny_skia/wide/Mod.h"

namespace tiny_skia::wide {

float fasterMin(float lhs, float rhs) {
  if (rhs < lhs) {
    return rhs;
  }

  return lhs;
}

float fasterMax(float lhs, float rhs) {
  if (lhs < rhs) {
    return rhs;
  }

  return lhs;
}

}  // namespace tiny_skia::wide
