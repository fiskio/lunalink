#include "lunalink/signal/modulator.hpp"

namespace lunalink::signal {

ModulationStatus modulate_bpsk_i(const uint8_t *chips, uint16_t chip_count,
                                 int8_t data_symbol, int8_t *out) noexcept {
  if (chips == nullptr || out == nullptr) {
    return ModulationStatus::kNullInput;
  }
  if (data_symbol != 1 && data_symbol != -1) {
    return ModulationStatus::kInvalidSymbol;
  }
  for (uint16_t i = 0; i < chip_count; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (chips[i] > 1U) {
      return ModulationStatus::kInvalidChipValue;
    }
  }
  for (uint16_t i = 0; i < chip_count; ++i) {
    // Per spec section 2.3.3, Table 8: logic 0 -> +1, logic 1 -> -1
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[i] = static_cast<int8_t>((chips[i] != 0U ? int8_t{-1} : int8_t{1}) *
                                 data_symbol);
  }
  return ModulationStatus::kOk;
}

ModulationStatus modulate_bpsk_q(const uint8_t *chips, uint16_t chip_count,
                                 int8_t *out) noexcept {
  if (chips == nullptr || out == nullptr) {
    return ModulationStatus::kNullInput;
  }
  for (uint16_t i = 0; i < chip_count; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (chips[i] > 1U) {
      return ModulationStatus::kInvalidChipValue;
    }
  }
  for (uint16_t i = 0; i < chip_count; ++i) {
    // Per spec section 2.3.3, Table 8: logic 0 -> +1, logic 1 -> -1
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[i] = chips[i] != 0U ? int8_t{-1} : int8_t{1};
  }
  return ModulationStatus::kOk;
}

} // namespace lunalink::signal
