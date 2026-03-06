# Specs Ambiguities and Assumptions

This document records implementation assumptions where AD1 Vol-A (LSIS V1.0, 29 January 2025) is ambiguous, interim, or explicitly marked TBD/TBC.

## 1. AFS-Q Code Assignment Beyond Interim Table 11

- Spec context:
  - Section 2.3.5.4 and LSIS-260 state final matched-code assignment and tertiary phasing are TBD (`LSIS-TBD-2001`).
  - Table 11 provides interim assignments only for LNSP Node IDs 1-12.
- Assumption applied:
  - The default API `tiered_code_epoch(prn_id, epoch_idx)` is restricted to PRNs 1-12 only.
  - For PRNs outside 1-12, callers must use explicit assignment API:
    - `tiered_code_epoch_assigned(primary_prn, secondary_code_idx, tertiary_prn, tertiary_phase_offset, epoch_idx)`.
- Rationale:
  - Avoid silently extrapolating interim mappings to unspecified PRNs.

## 2. AFS-Q Tertiary Phase Offset

- Spec context:
  - Section 2.3.5.4 notes tertiary phasing is TBD in future release.
  - Table 11 interim values use phase offset `0` for listed PRNs.
- Assumption applied:
  - Default mapping uses phase offset `0` for interim PRNs 1-12.
  - Explicit API allows non-zero phase offsets for forward compatibility.

## 3. Baseband IQ Power Scaling vs. Normalized Samples

- Spec context:
  - LSIS-103 defines 50/50 I/Q relative power.
  - LSIS-130 includes component power terms in the signal-generation equations.
- Assumption applied:
  - Current C4 implementation outputs normalized baseband channels with unit amplitude (`{-1, +1}`) and 5x I upsampling to match Q chip rate.
  - Absolute power calibration and dynamic transmit power control are treated as a downstream stage outside this module.
- Rationale:
  - Keeps deterministic digital baseband generation separate from RF/payload power-control implementation.

