#include "lunalink/signal/modulator.hpp"

#include <cassert>

namespace lunalink::signal {

void modulate_bpsk_i(const uint8_t *chips, uint16_t chip_count,
                     int8_t data_symbol, int8_t *out) noexcept {
  assert(chips != nullptr);
  assert(out != nullptr);
  assert(data_symbol == 1 || data_symbol == -1);
  for (uint16_t i = 0; i < chip_count; ++i) {
    // Per spec section 2.3.3, Table 8: logic 0 -> +1, logic 1 -> -1
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[i] = static_cast<int8_t>((chips[i] != 0U ? int8_t{-1} : int8_t{1}) *
                                 data_symbol);
  }
}

} // namespace lunalink::signal
