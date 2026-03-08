---
shaping: true
---

# LunaLink — AFS Reference Implementation — Shaping

## Gateway Philosophy (The Gateway Principle)

This project uses a **gateway-based approach** to break down the complex LSIS-AFS implementation into manageable, incremental milestones. 

1.  **Functional Capability:** Each gateway delivers a complete, independently testable feature.
2.  **Tangible Deliverables:** Every gateway produces code, test vectors, and documentation.
3.  **Validation Checkpoint:** Quality over speed; each gateway must be fully validated before proceeding.
4.  **Parallel Development:** Independent gateways (e.g., G1-G4) can be developed concurrently.

---

## Requirements (R)

| ID | Requirement | Status |
|----|-------------|--------|
| R0 | Best reference implementation: correctness and interoperability above all | Core goal |
| R1 | Spreading codes matching Annex3; Generation time < 1s/PRN | Must-have |
| R2 | Encode + decode: BCH(51,8), CRC-24, LDPC rate 1/2, Block Interleaver | Must-have |
| R2.1 | Encoding throughput: < 100ms per frame | NFR-1.2 |
| R2.2 | Decoding throughput: < 1s per frame | NFR-1.3 |
| R2.3 | Accuracy: BER < 10⁻⁵ at SNR > 0 dB | NFR-2.2 |
| R3 | All 22 MSG types; 8 spec-defined + 14 SISICD-defined | Must-have |
| R4 | C++ self-standing; flight-heritage coding; no heap in critical paths; ≥90% cov | Must-have |
| R5 | Documentation: Setup, Usage, API, Design, and Test results (Gateways 1-8) | Must-have |
| R7 | Validate against spec and demonstrate interoperability | Must-have |
| R7.1 | Publish test vectors in standardized binary format for cross-team interop | Must-have |
| R7.3 | Cross-decode: successful decoding of own and external test vectors | Must-have |
| R7.5 | Spec Findings Report: document ambiguities or errors in LSIS V1.0 | Must-have |
| R8 | Real-time capable: process faster than signal duration (12s) | NFR-1.4 |

---

## Selected Shape: Competition Gateway Alignment

This section defines the target technical shape. Implementation status is tracked in `docs/shaping/progress.md`.

| Gateway | Our ID | Mechanism |
|:---:|:---:|---|
| **G1** | **GD1.1-2** | **Spreading:** Parse hex PRN tables into static storage; tiered XOR assembly; < 1s/PRN. |
| **G4** | **GD4.1-2** | **Signal Gen:** BPSK(1) I / BPSK(5) Q; mapping: 0→+1, 1→-1; 5.115 MSPS; chunked I/O. |
| **G2** | **GD2.1** | **BCH(51,8):** 8-stage Fibonacci LFSR (Poly 763₈); decoder uses 256-hypothesis correlation. |
| **G2** | **GD2.4** | **CRC-24:** G(X) = (1+X)·P(X) per LSIS-FID0-467; applies to all subframes. |
| **G2** | **GD2.3** | **Interleaver:** 60x98 block matrix; row-wise write, column-wise read. |
| **G2** | **GD2.2** | **LDPC:** Min-sum BP (50 iter); SF2/3/4 matrices from Annex1; static buffers ~70KB. |
| **G3** | **GD3.1-2** | **Framing:** 12s duration; Prepend 68-sym sync; Subframe 1-4 builders; Frame assembly. |
| **G5** | **GD5.1-2** | **RX Sync:** Cross-correlation with sync pattern; handle timing offsets and Doppler. |
| **G6** | **GD6.1-2** | **Parsing:** Packed bit-field extraction; ToT calculation (WN×Tw + ITOW×Tbi + TOI×Tf). |
| **G7** | **GD7.1-4** | **Validation:** Round-trip tests; Performance benchmarking; Interop with test vectors. |
| **G8** | **GD8.1-4** | **Docs:** Sphinx-based API docs; Doxygen-style comments; usage examples for each gateway. |

---
## Performance Benchmarking Strategy (GD7.3)

To fulfill the competition's performance NFRs (NFR-1.1 through NFR-1.4), the implementation includes a dedicated benchmarking suite.

### 1. Metrics & Targets
| Metric | Description | Gateway | Target |
|:---|:---|:---:|:---|
| **Gen Latency** | Time to generate one 10230-chip PRN | G1 | < 1.0 s |
| **Enc Throughput** | Frames per second (FPS) for full 12s frame | G2 | > 10 FPS |
| **Dec Latency** | Time to decode one 6000-symbol frame | G2 | < 1.0 s |
| **Real-Time Factor** | Signal Duration / Processing Time | G7 | > 12.0 |

---

## Fit Check: Requirements × Deliverables

| Req | Requirement | Current Status | ID |
|-----|-------------|--------|---|
| R0 | Best reference implementation: correctness, interoperability, docs | In progress | program objective |
| R1 | Spreading codes from provided PRN hex tables | Implemented | ✅ GD1.1 |
| R2 | Encode + decode: BCH, CRC-24, LDPC, interleaver/deinterleaver, 12 s frame | In progress | GD2.1-4 |
| R3 | All 22 MSG types; 8 per spec + 14 per SISICD | Not started | GD6.1 |
| R4 | C++ self-standing, flight-heritage coding practices throughout | Partial | core signal path done |
| R5 | Documentation: Setup, Usage, API, Design, and Test results | Partial | GD8.1-4 |
| R7 | Validate against spec and demonstrate interoperability | Not started | GD7.4 |
| R7.1 | Test vectors in standardized binary format | Started | GD7.4 |
| R7.3 | Cross-decode: full RX pipeline decodes own/external vectors | Not started | GD7.4 |
| R7.5 | Spec Findings Report | In progress | GD8.4 |
| R8 | Real-time capable: process faster than signal duration | Not started | NFR-1.4 |

**Notes:**
- This table is strictly aligned with `docs/shaping/progress.md`.
- GD2.2 and GD5.1 spikes are closed at design level; implementation is ongoing.
- Gateway 7 (Integration) is the final validation hurdle for Level 4 Compliance.
