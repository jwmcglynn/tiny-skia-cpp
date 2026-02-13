#include "tiny_skia/AlphaRuns.h"

#include <limits>

namespace tiny_skia {

namespace {

AlphaRun makeRun(std::size_t value) {
  assert(value <= std::numeric_limits<std::uint16_t>::max());
  if (value == 0) {
    return std::nullopt;
  }
  return AlphaRun{static_cast<std::uint16_t>(value)};
}

}  // namespace

AlphaRuns::AlphaRuns(LengthU32 width) : runs(width + 1), alpha(width + 1, 0) {
  reset(width);
}

std::uint8_t AlphaRuns::catchOverflow(std::uint16_t alpha) {
  assert(alpha <= 256);
  return static_cast<std::uint8_t>(alpha - (alpha >> 8));
}

bool AlphaRuns::isEmpty() const {
  assert(runs[0].has_value());
  if (const auto run = runs[0]; run.has_value()) {
    return alpha[0] == 0 &&
           !runs[static_cast<std::size_t>(run.value())].has_value();
  }
  return true;
}

void AlphaRuns::reset(LengthU32 width) {
  assert(width > 0);
  assert(width <= std::numeric_limits<std::uint16_t>::max());
  const auto run = static_cast<std::size_t>(width);

  if (runs.size() < static_cast<std::size_t>(width) + 1) {
    runs.resize(width + 1);
    alpha.resize(width + 1);
  }

  runs[0] = makeRun(run);
  runs[static_cast<std::size_t>(width)] = std::nullopt;
  alpha[0] = 0;
}

std::size_t AlphaRuns::add(std::uint32_t x,
                           AlphaU8 start_alpha,
                           std::size_t middle_count,
                           AlphaU8 stop_alpha,
                           std::uint8_t max_value,
                           std::size_t offset_x) {
  std::size_t x_usize = static_cast<std::size_t>(x);

  std::size_t runs_offset = offset_x;
  std::size_t alpha_offset = offset_x;
  std::size_t last_alpha_offset = offset_x;
  assert(x_usize >= offset_x);
  x_usize -= offset_x;

  if (start_alpha != 0) {
    breakRun(std::span{runs}.subspan(runs_offset),
              std::span{alpha}.subspan(alpha_offset),
              x_usize,
              1);

    const std::uint16_t tmp =
        static_cast<std::uint16_t>(alpha[alpha_offset + x_usize] + start_alpha);
    assert(tmp <= 256);
    alpha[alpha_offset + x_usize] = catchOverflow(tmp);

    runs_offset += x_usize + 1;
    alpha_offset += x_usize + 1;
    x_usize = 0;
  }

  if (middle_count != 0) {
    breakRun(std::span{runs}.subspan(runs_offset),
              std::span{alpha}.subspan(alpha_offset),
              x_usize,
              middle_count);

    alpha_offset += x_usize;
    runs_offset += x_usize;
    x_usize = 0;
    do {
      alpha[alpha_offset] = catchOverflow(
          static_cast<std::uint16_t>(static_cast<std::uint16_t>(alpha[alpha_offset]) +
                                     max_value));
      const std::size_t n =
          static_cast<std::size_t>(runs[runs_offset].value());
      assert(n <= middle_count);
      alpha_offset += n;
      runs_offset += n;
      middle_count -= n;
      if (middle_count == 0) {
        break;
      }
    } while (true);

    last_alpha_offset = alpha_offset;
  }

  if (stop_alpha != 0) {
    breakRun(std::span{runs}.subspan(runs_offset),
              std::span{alpha}.subspan(alpha_offset),
              x_usize,
              1);
    alpha_offset += x_usize;
    alpha[alpha_offset] += stop_alpha;
    last_alpha_offset = alpha_offset;
  }

  return last_alpha_offset;
}

void AlphaRuns::breakRun(std::span<AlphaRun> runs,
                         std::span<std::uint8_t> alpha,
                         std::size_t x,
                         std::size_t count) {
  assert(count > 0);

  const std::size_t orig_x = x;
  std::size_t runs_offset = 0;
  std::size_t alpha_offset = 0;

  while (x > 0) {
    const std::size_t n = static_cast<std::size_t>(runs[runs_offset].value());
    assert(n > 0);

    if (x < n) {
      alpha[alpha_offset + x] = alpha[alpha_offset];
      runs[runs_offset] = makeRun(x);
      runs[runs_offset + x] = makeRun(n - x);
      break;
    }

    runs_offset += n;
    alpha_offset += n;
    x -= n;
  }

  runs_offset = orig_x;
  alpha_offset = orig_x;
  x = count;
  while (true) {
    const std::size_t n = static_cast<std::size_t>(runs[runs_offset].value());
    assert(n > 0);

    if (x < n) {
      alpha[alpha_offset + x] = alpha[alpha_offset];
      runs[runs_offset] = makeRun(x);
      runs[runs_offset + x] = makeRun(n - x);
      break;
    }

    x -= n;
    if (x == 0) {
      break;
    }

    runs_offset += n;
    alpha_offset += n;
  }
}

void AlphaRuns::breakAt(std::span<std::uint8_t> alpha,
                        std::span<AlphaRun> runs,
                        std::int32_t x) {
  std::size_t alpha_i = 0;
  std::size_t run_i = 0;
  while (x > 0) {
    const auto n = runs[run_i].value();
    const std::size_t n_usize = static_cast<std::size_t>(n);
    const std::int32_t n_i32 = static_cast<std::int32_t>(n);
    if (x < n_i32) {
      alpha[alpha_i + static_cast<std::size_t>(x)] = alpha[alpha_i];
      runs[0] = makeRun(static_cast<std::size_t>(x));
      runs[static_cast<std::size_t>(x)] = makeRun(static_cast<std::size_t>(n_i32 - x));
      break;
    }

    run_i += n_usize;
    alpha_i += n_usize;
    x -= n_i32;
  }
}

}  // namespace tiny_skia
