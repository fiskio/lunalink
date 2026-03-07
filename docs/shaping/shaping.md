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

## Selected Shape: C — Eigen for PNT, static C++ for LDPC

This section defines the target technical shape. Implementation status is tracked in `docs/shaping/progress.md`.

| Gateway | Part | Mechanism |
|:---:|---|---|
| **G1** | C1, C3 | **Spreading:** Parse hex PRN tables into static storage; tiered XOR assembly; < 1s/PRN. |
| **G4** | C2, C4 | **Signal Gen:** BPSK(1) I / BPSK(5) Q; mapping: 0→+1, 1→-1; 5.115 MSPS; chunked I/O. |
| **G2** | C5 | **BCH(51,8):** 8-stage Fibonacci LFSR (Poly 763₈); decoder uses 256-hypothesis correlation. |
| **G2** | — | **CRC-24:** G(X) = (1+X)·P(X) per LSIS-FID0-467; applies to all subframes. |
| **G2** | — | **Interleaver:** 60x98 block matrix; row-wise write, column-wise read. |
| **G2** | C6, C7 | **LDPC:** Min-sum BP (50 iter); SF2/3/4 matrices from Annex1; static buffers ~70KB. |
| **G3** | C8 | **Framing:** 12s duration; Prepend 68-sym sync; Subframe 1-4 builders; Frame assembly. |
| **G5** | — | **RX Sync:** Cross-correlation with sync pattern; handle timing offsets and Doppler. |
| **G6** | C9 | **Parsing:** Packed bit-field extraction; ToT calculation (WN×Tw + ITOW×Tbi + TOI×Tf). |
| **G7** | — | **Validation:** Round-trip tests; Performance benchmarking; Interop with test vectors. |
| **G8** | — | **Docs:** Sphinx-based API docs; Doxygen-style comments; usage examples for each gateway. |

---
## Performance Benchmarking Strategy (G7.3)

To fulfill the competition's performance NFRs (NFR-1.1 through NFR-1.4), the implementation includes a dedicated benchmarking suite.

### 1. Metrics & Targets
| Metric | Description | Gateway | Target |
|:---|:---|:---:|:---|
| **Gen Latency** | Time to generate one 10230-chip PRN | G1 | < 1.0 s |
| **Enc Throughput** | Frames per second (FPS) for full 12s frame | G2 | > 10 FPS |
| **Dec Latency** | Time to decode one 6000-symbol frame | G2 | < 1.0 s |
| **Real-Time Factor** | Signal Duration / Processing Time | G7 | > 12.0 |

### 2. Methodology
*   **Environment:** Benchmarks are executed on a single thread with no hardware acceleration (SIMD/GPU) enabled by default, to ensure baseline compliance.
*   **Warm-up:** Each test runs 10 "warm-up" iterations before measuring to exclude OS/Cache noise.
*   **Tools:**
    *   **C++:** `google/benchmark` (if available) or `std::chrono` high-resolution timers.
    *   **Python:** `timeit` and `cProfile` for binding overhead analysis.

### 3. Reporting
A `performance_report.json` is generated during CI/CD (Phase 3), documenting:
1.  System hardware specs (CPU, RAM).
2.  Mean, Min, Max, and Standard Deviation for each metric.
3.  A "Pass/Fail" status against Gateway success criteria.

| Req | Requirement | Current Status | C |
|-----|-------------|--------|---|
| R0 | Best reference implementation: correctness, interoperability, docs | In progress | program objective |
| R1 | Spreading codes from provided PRN hex tables | Implemented | ✅ via C1 |
| R2 | Encode + decode: BCH, CRC-24, LDPC, interleaver/deinterleaver, 12 s frame | Not implemented yet | planned via C5–C8 |
| R3 | All 22 MSG types; 8 per spec (G1/G2/G4/G5/G8/G22/G24/G30), 14 per SISICD | Not implemented yet | planned via C9 |
| R4 | C++ self-standing, flight-heritage coding practices throughout | Partial | core signal path in place; extend across codec/RX |
| R5 | Documentation: compliance matrix, block diagrams, narrative Sphinx pages | Partial | signal-chain docs present; compliance/SISICD/spec-findings incomplete |
| R6 | Web-app demo (Moon globe, correlator, PNT, optionally SDR) | Deferred | C12/C15/C16 deferred |
| R7 | Validate against spec and demonstrate interoperability | Not implemented yet | planned V3/V4 |
| R7.1 | Test vectors in documented binary format | Started | test-vector format draft exists |
| R7.2 | BER curves vs uncoded BPSK + Shannon limit | Not implemented yet | planned V4 |
| R7.3 | Cross-decode: full RX pipeline decodes own test vectors | Not implemented yet | planned V4 |
| R7.4 | SISICD for TBW message types | Not implemented yet | planned draft V2, final V4 |
| R7.5 | Spec Findings Report | In progress | ambiguities log started; draft/final pending |
| R7.6 | Parameter sensitivity testing | Not implemented yet | planned V4 |
| R7.7 | Link budget analysis (spec power levels → Eb/N₀ → BER → decode rate) | Not implemented yet | planned V4 |
| R8 | PNT pipeline: correlator → pseudoranges → position fix | Not implemented yet | planned V5 (C10/C11/C13) |

**Notes:**
- This table is intentionally status-aware; it must stay aligned with `docs/shaping/progress.md`.
- R6 remains a deliberate defer: competition strength is standards-faithful implementation and interop evidence first.
- C6/C7 and C10 design spikes are closed at design level, but implementation status remains tracked in `progress.md`.
- V1–V4 cover milestone intent; actual completion is based on landed code/tests/docs, not planned architecture text.

**Open spikes:** spike-C12.md (deferred)
