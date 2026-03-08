---
shaping: true
---

# LunaLink — AFS Reference Implementation — Slices

## Competition Timeline & Gateways

This project follows the **Gateway Principle** defined in the competition documents. Each gateway represents a functional capability that must be fully validated.

| Gateway | Deliverable | Success Criteria | Status |
|:---|:---|:---|:---|
| **G1** | Spreading Code Generation | All 210 PRNs match Annex3; Gen time < 1s/PRN | Implemented [Hardened] |
| **G2** | FEC Encoding & Decoding | Round-trip recovery; BER < 10⁻⁵ @ SNR > 0dB | In progress |
| **G3** | Message Framing | 12s frame assembly; 6000 symbol count; Sync pass | In progress |
| **G4** | Baseband Signal Generation | Binary I/Q files; 5.115 MSPS; Dual-channel BPSK | Implemented [Hardened] |
| **G5** | Frame Sync & Decoding | >99% detection @ SNR > 0dB; Sync offsets handled | Not started |
| **G6** | Message Parsing | All subframes parsed; ToT calculation accuracy | Not started |
| **G7** | Integration & Validation | 100% round-trip accuracy; Interop tests pass | Not started |
| **G8** | Documentation | Complete API docs + Setup/Usage examples | Partial |

---

## Phase 1 — Standards Compliance & Architecture (March 2026)

**Goal:** Establish the foundation and validate core signal components.

**Gateways Covered:** G1 (Implemented [Hardened]), G4 (Implemented [Hardened]), Partial G2 (Encoder/Decoder Implemented [Hardened]), G8.

*   **Signal Foundation:** Gold-2046 generation (GD1.1), AFS-I BPSK modulation (GD4.1), and IQ Multiplexing (GD4.2). Implemented [Hardened] with TMR and CFI.
*   **FEC Core:** Flight-hardened BCH(51,8) encoder/decoder (GD2.1) and CSR-optimized LDPC encoder (GD2.2). Implemented [Hardened] with ALU mirroring and stack scrubbing.
*   **Architecture:** Module layout defined in `docs/signal/architecture.rst` (GD8.1).
*   **Compliance:** Skeleton matrix in `docs/signal/compliance_matrix.rst` (GD8.3).
*   **Validation:** C++ Catch2 suite + Python pytest integration.

---

## Phase 2 — Protocol Implementation (April - July 2026)

**Goal:** Complete the full signal chain (TX and RX).

### April: Spreading & Encoding (G1, G2)
*   **G1 Complete:** Weil-10230 and Weil-1500 generation; Secondary code mapping; Tiered assembly.
*   **G2 Partial:** BCH(51,8) encoder/decoder; CRC-24 generator; Block Interleaver.

### May: Framing & Baseband (G3, G4)
*   **G3 Complete:** Subframe 1-4 builders; Frame assembler; 12s timing validation.
*   **G4 Complete:** Signal file export (bin/csv); Configurable sample rates.

### June: Decoding Infrastructure (G2, G5)
*   **G2 Complete:** LDPC encoder/decoder (5G NR matrices); Min-sum BP implementation.
*   **G5 Complete:** Sync pattern detection; Symbol extraction; Deinterleaver.

### July: Message Parsing & Integration (G6, G7)
*   **G6 Complete:** All 22 subframe parsers; Time of Transmission (ToT) calculator.
*   **G7 Start:** Initial round-trip testing (encode -> frame -> signal -> decode -> verify).

---

## Phase 3 — Validation & Interoperability (August 2026)

**Goal:** Final verification and interoperability demonstration.

**Gateways Covered:** G7, G8.

*   **Interop:** Cross-decoding signals from other teams using standardized test vector format.
*   **Performance:** Benchmark encoding (<100ms) and decoding (<1s) throughput.
*   **Validation:** Final BER curves; Compliance Report generation.
*   **Delivery:** Cleanup documentation, examples, and build instructions.
