#include "tiny_skia/Mask.h"

#include <cstddef>
#include <utility>

namespace tiny_skia {

std::optional<Mask> Mask::fromSize(std::uint32_t width, std::uint32_t height) {
  const auto size = IntSize::fromWh(width, height);
  if (!size.has_value()) {
    return std::nullopt;
  }

  const auto data_len = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  return Mask(std::vector<std::uint8_t>(data_len, 0), size.value());
}

std::optional<Mask> Mask::fromVec(std::vector<std::uint8_t> data, IntSize size) {
  const auto data_len =
      static_cast<std::size_t>(size.width()) * static_cast<std::size_t>(size.height());
  if (data.size() != data_len) {
    return std::nullopt;
  }

  return Mask(std::move(data), size);
}

std::vector<std::uint8_t> Mask::take() {
  size_ = IntSize{};
  return std::move(data_);
}

}  // namespace tiny_skia
