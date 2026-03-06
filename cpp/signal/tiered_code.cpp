#include "lunalink/signal/tiered_code.hpp"

#include <cassert>

#include "lunalink/signal/prn.hpp"

namespace lunalink::signal {

void tiered_code_epoch(uint8_t prn_id, uint16_t epoch_idx,
                       uint8_t *out) noexcept {
  assert(prn_id >= 1 && prn_id <= kPrnCount);
  assert(epoch_idx < kEpochsPerFrame);
  assert(out != nullptr);

  const uint8_t *primary_packed = weil10230_prn_packed(prn_id);
  const uint8_t *tertiary_packed = weil1500_prn_packed(prn_id);

  // Secondary chip for this epoch (one secondary chip per primary epoch).
  const uint8_t sec_idx = secondary_code_index(prn_id);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  const uint8_t sec_chip = kSecondaryCodes[sec_idx][epoch_idx % kSecondaryCodeLength];

  // Tertiary chip (one tertiary chip per Ns=4 primary epochs).
  const auto tert_idx =
      static_cast<uint16_t>(epoch_idx / kSecondaryCodeLength);
  const uint8_t tert_chip = unpack_chip(tertiary_packed, tert_idx);

  // For the entire epoch, sec_chip and tert_chip are constant, so the
  // modifier (sec XOR tert) is a single bit we XOR into every primary chip.
  const auto modifier = static_cast<uint8_t>(sec_chip ^ tert_chip);

  for (uint16_t i = 0; i < kWeil10230ChipLength; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[i] = static_cast<uint8_t>(unpack_chip(primary_packed, i) ^ modifier);
  }
}

} // namespace lunalink::signal
