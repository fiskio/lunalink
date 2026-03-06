#pragma once
#include <cstdint>
#include <span>

#include "lunalink/signal/prn.hpp"

namespace lunalink::signal {

enum class TieredCodeStatus : uint8_t {
  kOk = 0,
  kInvalidPrn,
  kInvalidEpoch,
  kOutputTooSmall,
  kInvalidAssignment,
};

/// Secondary code definitions per LSIS V1.0 §2.3.5.3.2, Table 10.
inline constexpr uint8_t kSecondaryCodeLength = 4;
inline constexpr uint8_t kSecondaryCodeCount  = 4;
inline constexpr uint8_t kInterimAssignmentMaxPrn = 12;

// NOLINTNEXTLINE(hicpp-avoid-c-arrays)
inline constexpr uint8_t kSecondaryCodes[4][4] = {
    {1, 1, 1, 0},  // S0
    {0, 1, 1, 1},  // S1
    {1, 0, 1, 1},  // S2
    {1, 1, 0, 1},  // S3
};

/// Total primary epochs per 12 s frame: Ns × NT = 4 × 1500 = 6000.
inline constexpr uint16_t kEpochsPerFrame =
    static_cast<uint16_t>(static_cast<uint32_t>(kSecondaryCodeLength) *
                          kWeil1500ChipLength);

/// Return the secondary code index for a given PRN ID.
/// Per Table 11 interim assignments: PRN i → S_{(i-1) mod 4}.
[[nodiscard]] constexpr TieredCodeStatus secondary_code_index_checked(
    PrnId    prn_id,
    uint8_t* out_idx) noexcept {
  if (out_idx == nullptr) [[unlikely]] {
    return TieredCodeStatus::kInvalidAssignment;
  }
  if (!prn_id.valid()) [[unlikely]] {
    return TieredCodeStatus::kInvalidPrn;
  }
  *out_idx = static_cast<uint8_t>((static_cast<uint8_t>(prn_id) - 1U) % kSecondaryCodeCount);
  return TieredCodeStatus::kOk;
}

/// True when PRN is covered by LSIS V1.0 interim Table 11 mapping.
[[nodiscard]] inline constexpr bool is_interim_prn(PrnId prn_id) noexcept {
  return prn_id.valid() && static_cast<uint8_t>(prn_id) <= kInterimAssignmentMaxPrn;
}

/// Tiered code component assignment for one LNSP node.
struct TieredCodeAssignment {
  PrnId    primary_prn = PrnId{1U};
  uint8_t  secondary_code_idx = 0U;
  PrnId    tertiary_prn = PrnId{1U};
  uint16_t tertiary_phase_offset = 0U;
};

/// Validate an assignment.
[[nodiscard]] inline constexpr bool valid_tiered_assignment(
    const TieredCodeAssignment& a) noexcept {
  return a.primary_prn.valid() &&
         a.secondary_code_idx < kSecondaryCodeCount &&
         a.tertiary_prn.valid() &&
         a.tertiary_phase_offset < kWeil1500ChipLength;
}

/// Build the interim test assignment from LSIS V1.0 Table 11.
[[nodiscard]] constexpr TieredCodeStatus default_tiered_assignment_checked(
    PrnId                 prn_id,
    TieredCodeAssignment* out_assignment) noexcept {
  if (out_assignment == nullptr) [[unlikely]] {
    return TieredCodeStatus::kInvalidAssignment;
  }
  if (!is_interim_prn(prn_id)) [[unlikely]] {
    return TieredCodeStatus::kInvalidPrn;
  }
  uint8_t secondary_idx = 0;
  const auto secondary_status = secondary_code_index_checked(
      prn_id,
      &secondary_idx);
  if (secondary_status != TieredCodeStatus::kOk) {
    return secondary_status;
  }
  *out_assignment = TieredCodeAssignment{
      prn_id,
      secondary_idx,
      prn_id,
      0U,
  };
  return TieredCodeStatus::kOk;
}

/// Generate one primary epoch (10 230 chips) of the tiered AFS-Q code.
///
/// For the given PRN and epoch index, produces:
///   out[i] = primary[i] XOR secondary[epoch % 4] XOR tertiary[epoch / 4]
/// per LSIS V1.0 §2.3.5.2 (C_Tiered = Cp ⊕ Cs ⊕ CT).
///
/// @pre prn_id is valid
/// @pre epoch_idx < kEpochsPerFrame (6000)
/// @pre out.size() == kWeil10230ChipLength (10230)
/// @post out is populated with valid tiered chips if kOk
/// @complexity O(kWeil10230ChipLength)
[[nodiscard]] TieredCodeStatus tiered_code_epoch(
    PrnId              prn_id,
    uint16_t           epoch_idx,
    std::span<uint8_t> out) noexcept;

/// Checked version of tiered_code_epoch using a specific assignment.
/// @pre valid_tiered_assignment(assignment)
/// @pre epoch_idx < kEpochsPerFrame (6000)
/// @pre out.size() == kWeil10230ChipLength (10230)
[[nodiscard]] TieredCodeStatus tiered_code_epoch_checked(
    const TieredCodeAssignment& assignment,
    uint16_t                    epoch_idx,
    std::span<uint8_t>          out) noexcept;

/**
 * @brief Ergonomic overloads.
 */
template <typename T>
[[nodiscard]] inline TieredCodeStatus tiered_code_epoch(
    PrnId    prn_id,
    uint16_t epoch_idx,
    T&&      out) noexcept {
  return tiered_code_epoch(prn_id, epoch_idx, std::span<uint8_t>(out));
}

template <typename T>
[[nodiscard]] inline TieredCodeStatus tiered_code_epoch_checked(
    const TieredCodeAssignment& assignment,
    uint16_t                    epoch_idx,
    T&&                         out) noexcept {
  return tiered_code_epoch_checked(assignment, epoch_idx, std::span<uint8_t>(out));
}

} // namespace lunalink::signal
