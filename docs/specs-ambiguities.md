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

## 4. Safety Policy for Invalid Inputs in C++ APIs

- Engineering context:
  - In this project, silent failure is treated as a safety risk for signal-generation
    software.
- Assumption applied:
  - Public C++ signal APIs use explicit status returns (enums) and ``[[nodiscard]]``
    instead of assert-only preconditions for runtime input validation.
  - Python bindings translate non-OK statuses into exceptions.
- Rationale:
  - Prevent undefined behavior in release builds and make all invalid-input handling
    explicit and testable.

## 5. Baseband IQ Digital Container Format

- Spec context:
  - LSIS-130 defines signal generation using distinct in-phase and quadrature
    components (I and Q) in the modulation equation.
  - The document does not prescribe a single in-memory software container format
    for digital baseband sample transport between modules.
- Assumption applied:
  - C4 outputs baseband as interleaved IQ int16 pairs:
    - ``[I0, Q0, I1, Q1, ...]`` in C++
    - ``shape (10230, 2)`` in Python, columns ``[I, Q]``
  - This is treated as a representation choice for the LSIS I/Q model, not a
    change to the signal definition.
- Rationale:
  - Preserves explicit I/Q structure, avoids ambiguity with scalar ``I+Q``
    interpretations, and matches common SDR/DMA interface expectations.

## 6. BCH(51,8) Generator Polynomial: Text vs. Diagram Mismatch

- Spec context:
  - Section 2.4.2.1 states the generator polynomial is "763 (octal)".
  - 763₈ = 0b111110011 = x⁸+x⁷+x⁶+x⁵+x⁴+x+1.
  - Figure 7 labels the polynomial as 1+X³+X⁴+X⁵+X⁶+X⁷+X⁸ (= 0b111111001 = 771₈).
  - These differ at the x¹ vs x³ term.
- Resolution:
  - The Figure 8 test vector (SB1=0x045 → encoded=0x229f61dbb84a0) was used as
    ground truth.
  - A Fibonacci LFSR with the **Figure 7 polynomial** (1+X³+X⁴+X⁵+X⁶+X⁷+X⁸)
    reproduces the test vector exactly.
  - The text value "763 octal" does **not** reproduce the test vector under any
    standard LFSR configuration (Galois or Fibonacci, forward or reversed state).
- Assumption applied:
  - The implementation uses the polynomial from Figure 7: 1+X³+X⁴+X⁵+X⁶+X⁷+X⁸.
  - The text "763 (octal)" is treated as an editorial error.
- Rationale:
  - The test vector is the most authoritative evidence of intended behavior.
