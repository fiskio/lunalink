#include "lunalink/signal/frame.hpp"

#include "lunalink/signal/bch.hpp"

#include <cstring>

namespace lunalink::signal {

// Sync pattern: CC63F74536F49E04A (hex), 68 bits MSB-first.
// LSIS V1.0 §2.4.1, Table 12.
// NOLINTNEXTLINE(hicpp-avoid-c-arrays)
const uint8_t kSyncPattern[kSyncLength] = {
    // C        C        6        3
    1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1,
    // F        7        4        5
    1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1,
    // 3        6        F        4
    0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0,
    // 9        E        0        4
    1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    // A (4 bits)
    1, 0, 1, 0,
};

FrameStatus frame_build_partial(
    uint8_t     fid,
    uint8_t     toi,
    uint8_t*    out,
    std::size_t out_len
) noexcept {
  if (out == nullptr) {
    return FrameStatus::kNullOutput;
  }
  if (out_len < kFrameLength) {
    return FrameStatus::kOutputTooSmall;
  }

  // 1. Copy sync pattern (68 symbols).
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
  std::memcpy(out, kSyncPattern, kSyncLength);

  // 2. BCH-encode SB1 into frame at offset 68.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto bch_status = bch_encode(fid, toi, out + kSyncLength, kSb1Length);
  if (bch_status != BchStatus::kOk) {
    return FrameStatus::kBchFailed;
  }

  // 3. Zero-pad SB2+SB3+SB4 payload (5880 symbols).
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  std::memset(out + kSyncLength + kSb1Length, 0, kPayloadLength);

  return FrameStatus::kOk;
}

} // namespace lunalink::signal
