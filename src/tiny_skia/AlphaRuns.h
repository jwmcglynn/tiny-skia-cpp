#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tiny_skia {

using AlphaRun = std::optional<std::uint16_t>;
using AlphaU8 = std::uint8_t;
using LengthU32 = std::uint32_t;

class AlphaRuns {
 public:
  explicit AlphaRuns(LengthU32 width);

  static std::uint8_t catch_overflow(std::uint16_t alpha);
  [[nodiscard]] bool is_empty() const;

  void reset(LengthU32 width);

  std::size_t add(std::uint32_t x,
                  AlphaU8 start_alpha,
                  std::size_t middle_count,
                  AlphaU8 stop_alpha,
                  std::uint8_t max_value,
                  std::size_t offset_x);

  static void break_run(AlphaRun* runs,
                        std::uint8_t* alpha,
                        std::size_t x,
                        std::size_t count);
  static void break_at(std::uint8_t* alpha,
                       AlphaRun* runs,
                       std::int32_t x);

  std::vector<AlphaRun> runs;
  std::vector<std::uint8_t> alpha;
};

}  // namespace tiny_skia
