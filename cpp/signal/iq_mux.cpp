#include "lunalink/signal/iq_mux.hpp"

#include "lunalink/signal/prn.hpp"

namespace lunalink::signal {

bool multiplex_iq(const int8_t *i_samples, const int8_t *q_samples,
                  int16_t *out) noexcept {
  if (i_samples == nullptr || q_samples == nullptr || out == nullptr) {
    return false;
  }

  for (uint16_t n = 0; n < kIqSamplesPerEpoch; ++n) {
    // Upsample AFS-I: each I chip spans 5 Q-rate samples (sample-and-hold).
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-signed-char-misuse)
    const auto i_val = static_cast<int16_t>(i_samples[n / kIqUpsampleFactor]);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,bugprone-signed-char-misuse)
    const auto q_val = static_cast<int16_t>(q_samples[n]);

    // Interleaved IQ output: [I0, Q0, I1, Q1, ...]
    const auto idx = static_cast<uint32_t>(n) * 2U;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[idx] = i_val;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    out[idx + 1U] = q_val;
  }
  return true;
}

} // namespace lunalink::signal
