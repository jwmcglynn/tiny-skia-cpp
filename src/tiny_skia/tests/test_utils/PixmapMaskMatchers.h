#pragma once

#include <gmock/gmock.h>

#include <cstdint>
#include <optional>

#include "tiny_skia/Mask.h"
#include "tiny_skia/Pixmap.h"

namespace tiny_skia::tests::matchers {

inline ::testing::Matcher<const tiny_skia::PremultipliedColorU8&> PremultipliedColorU8Eq(
    std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
  using ::testing::AllOf;
  using ::testing::Eq;
  using ::testing::Property;

  return AllOf(Property("red", &tiny_skia::PremultipliedColorU8::red, Eq(red)),
               Property("green", &tiny_skia::PremultipliedColorU8::green, Eq(green)),
               Property("blue", &tiny_skia::PremultipliedColorU8::blue, Eq(blue)),
               Property("alpha", &tiny_skia::PremultipliedColorU8::alpha, Eq(alpha)));
}

inline ::testing::Matcher<const tiny_skia::SubMaskRef&> SubMaskRefEq(std::uint32_t width,
                                                                     std::uint32_t height,
                                                                     std::uint32_t realWidth) {
  using ::testing::AllOf;
  using ::testing::Eq;
  using ::testing::Field;
  using ::testing::ResultOf;

  return AllOf(Field("realWidth", &tiny_skia::SubMaskRef::realWidth, Eq(realWidth)),
               ResultOf(
                   "width", [](const tiny_skia::SubMaskRef& value) { return value.size.width(); },
                   Eq(width)),
               ResultOf(
                   "height", [](const tiny_skia::SubMaskRef& value) { return value.size.height(); },
                   Eq(height)));
}

inline ::testing::Matcher<const std::optional<tiny_skia::SubMaskRef>&> OptionalSubMaskRefEq(
    std::uint32_t width, std::uint32_t height, std::uint32_t realWidth) {
  return ::testing::Optional(SubMaskRefEq(width, height, realWidth));
}

inline ::testing::Matcher<const tiny_skia::SubMaskMut&> SubMaskMutEq(std::uint32_t width,
                                                                     std::uint32_t height,
                                                                     std::uint32_t realWidth) {
  using ::testing::AllOf;
  using ::testing::Eq;
  using ::testing::Field;
  using ::testing::ResultOf;

  return AllOf(Field("realWidth", &tiny_skia::SubMaskMut::realWidth, Eq(realWidth)),
               ResultOf(
                   "width", [](const tiny_skia::SubMaskMut& value) { return value.size.width(); },
                   Eq(width)),
               ResultOf(
                   "height", [](const tiny_skia::SubMaskMut& value) { return value.size.height(); },
                   Eq(height)));
}

inline ::testing::Matcher<const std::optional<tiny_skia::SubMaskMut>&> OptionalSubMaskMutEq(
    std::uint32_t width, std::uint32_t height, std::uint32_t realWidth) {
  return ::testing::Optional(SubMaskMutEq(width, height, realWidth));
}

inline ::testing::Matcher<const tiny_skia::SubPixmapMut&> SubPixmapMutEq(std::size_t width,
                                                                         std::size_t height,
                                                                         std::size_t realWidth) {
  using ::testing::AllOf;
  using ::testing::Eq;
  using ::testing::Field;
  using ::testing::Property;

  return AllOf(Field("realWidth", &tiny_skia::SubPixmapMut::realWidth, Eq(realWidth)),
               Property("width", &tiny_skia::SubPixmapMut::width, Eq(width)),
               Property("height", &tiny_skia::SubPixmapMut::height, Eq(height)));
}

inline ::testing::Matcher<const std::optional<tiny_skia::SubPixmapMut>&> OptionalSubPixmapMutEq(
    std::size_t width, std::size_t height, std::size_t realWidth) {
  return ::testing::Optional(SubPixmapMutEq(width, height, realWidth));
}

}  // namespace tiny_skia::tests::matchers
