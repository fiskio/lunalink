---
shaping: true
---

# LunaLink — AFS Reference Implementation — Shaping

## Requirements (R)

| ID | Requirement | Status |
|----|-------------|--------|
| R0 | Produce the best LSIS-AFS reference implementation: correctness and interoperability above all; compelling documentation; clean, auditable code | Core goal |
| R1 | Generate spreading codes by loading from the provided PRN hex tables (Gold-2046, Weil-10230, Weil-1500); no algorithmic construction required | Must-have |
| R2 | Implement navigation message encoding AND decoding: BCH(51,8) on Subframe 1, CRC-24 integrity check, 5G NR LDPC on Subframes 2–4, block interleaver/deinterleaver, 12-second frame structure | Must-have |
| R3 | Implement all 22 navigation message types defined in LSIS V1.0 Section 2.5; 8 are fully defined at bit level (G1, G2, G4, G5, G8, G22, G24, G30) — the remaining 14 are TBW in V1.0 and implemented with SISICD-defined field layouts | Must-have |
| R4 | C++ is the primary, self-standing implementation — all signal chain logic lives in C++; Python exposes it via pybind11 bindings; C++ code applies flight-heritage coding practices: fixed-width types, no heap allocation in critical paths, no exceptions in core, deterministic behaviour, hardening via sanitizers, clang-tidy (CERT + HICPP + CppCoreGuidelines + Fuchsia), ≥90% coverage | Must-have |
| R5 | Documentation explains the project, AFS signal theory, and design choices — accessible to someone who doesn't know GNSS; includes compliance matrix, signal chain block diagrams, narrative Sphinx pages; code itself is lean and production-quality, not pedagogical | Must-have |
| R6 | Deliver a web-app demo with 3D Moon globe, satellite orbits, correlator, PNT solver, and optionally Pluto+ SDR RF | Nice-to-have |
| R7 | Validate against the spec and demonstrate interoperability | Must-have |
| R7.1 | Publish test vectors: one complete encoded frame per MSG type, in a documented binary format designed for cross-team interoperability; format spec published early (V2) to enable other teams to adopt it; checksums included | Must-have |
| R7.2 | BER-vs-Eb/N₀ curves for SF2 and SF3, plotted against uncoded BPSK and Shannon limit | Must-have |
| R7.3 | Cross-decode: full RX pipeline (deinterleaver → LDPC decode → CRC-24 → message deserialise) successfully decodes own test vectors | Must-have |
| R7.4 | SISICD: Signal-In-Space Interface Control Document for our implementation, covering all field layouts for the 14 TBW message types | Must-have |
| R7.5 | Spec Findings Report: document any ambiguities, errors, or implementation choices found in LSIS V1.0 during development | Must-have |
| R7.6 | Parameter sensitivity testing: sweep key signal parameters (Eb/N₀, Doppler, code phase offset) and document impact on decode success rate | Must-have |
| R7.7 | Link budget analysis: connect spec receiver sensitivity (−160 dBW min, −147 dBW max per LSIS-110) to operational Eb/N₀, predicted BER, and frame decode success rate | Must-have |
| R8 | PNT pipeline: N satellites → composite IQ → software correlator → pseudoranges → position fix | Should-have |

---

## Selected Shape: C — Eigen for PNT, static C++ for LDPC, no external FEC framework

This section defines the target technical shape. Implementation status is tracked separately in `docs/shaping/progress.md`.

| Part | Mechanism | Flag |
|------|-----------|:----:|
| C1 | Code loader: parse hex PRN tables into `static constexpr uint8_t` chip arrays | |
| C2 | BPSK modulator: chip × data symbol → `int8_t` {−1, +1} IQ samples, fixed chip rates. **Chip mapping: logic 0 → +1, logic 1 → −1 (spec §2.3.3, Table 8)** | |
| C3 | Tiered code combiner: XOR primary + secondary + tertiary into composite AFS-Q; secondary codes S0=1110, S1=0111, S2=1011, S3=1101; PRN i → S_{(i-1) mod 4} (interim test assignment per §2.3.5.4 Table 11; full assignments deferred LSIS-TBD-2001) | |
| C4 | IQ multiplexer: 5x upsample AFS-I to Q rate and emit interleaved `int16_t` IQ pairs (`[I0,Q0,I1,Q1,...]`) with equal normalized digital amplitude; output rate 5.115 MSPS/channel (10.23 Msamples/s total I+Q values); chunked generation strategy for 12 s frames | |
| C5 | BCH(51,8) codec: SB1 carries 9 bits — FID (2 bits, frame ID) + TOI (7 bits, time of interval); bit 0 (FID MSB) is prepended raw, remaining 8 bits are BCH-encoded. **Encoder** — GF(2) polynomial division, generator polynomial 763 (octal); output is 52 symbols — the 51 encoded bits each XOR'd with bit 0, with bit 0 prepended; static LUT. **Decoder** — correlation-based (256 hypotheses, per spec §2.4.2.1); returns decoded 8-bit word + error flag | |
| C6 | LDPC encoder: sparse GF(2) matrix-vector products `p1 = B⁻¹·A·s`, `p2 = C·s + D·p1`; filler bits appended before encoding (0 for SB2, 10 zeros for SB3/SB4); systematic puncturing: first z=240 (SB2) or z=176 (SB3/SB4) bits dropped; remaining parity beyond `(p1;p2)` punctured; static arrays; SF2 ≤800 B, SF3 ≤600 B. **SB4 reuses SF3 matrices** (identical parameters: 1740 symbols, z=176, 10 filler bits; no separate SF4 matrices in spec). **Table pipeline:** `scripts/gen_ldpc_tables.py` reads 003a–003j CSVs → C++ static sparse arrays | |
| C7 | LDPC decoder: punctured positions restored as LLR=0 erasures; min-sum BP (~50 iterations, compile-time constant) on Tanner graph from H; static LLR buffers SF2 ~70 KB, SF3 ~55 KB; no heap | |
| C8 | Frame builder/parser: **TX** — prepend 68-symbol sync pattern (`CC63F74536F49E04A`) + SB1 (BCH, 52 symbols) + SB2–SB4 (LDPC-encoded, interleaved via 60×98 block interleaver) into 12 s frame, fixed-size buffer. **RX** — sync detection, block deinterleaver (inverse 60×98), LDPC decode, CRC-24 check (G(X)=(1+X)·P(X), spec LSIS-FID0-467), BCH decode, message deserialise; full receive path | |
| C9 | Message serialiser: all 22 MSG types → packed bit fields; 8 fully defined per spec (G1, G2, G4, G5, G8, G22, G24, G30); remaining 14 TBW in V1.0 — field layouts defined in our SISICD, `[[nodiscard]]` throughout | |
| C10 | Software correlator (AFS-I only): PCPS acquisition pre-run at startup (~1–5 s); DLL/PLL tracking at 37K MACs/ms for N=6; all static buffers ~30 KB; runs on software-simulated IQ, independent of Pluto+ RX | |
| C11 | PNT solver: `Eigen::Matrix<double, Dynamic, 4>` bounded N≤12; `ColPivHouseholderQR` least-squares; no heap for N≤12 with fixed max-size matrix | |
| C12 | Pluto+ interface: GNURadio OOT module (`gr-lunalink`) using gr-iio; `afs_source` block wraps `lunalink_core` → `gr_complex` IQ; gr-iio Pluto+ sink at 2492.028 MHz for TX; Pluto+ source → log-power FFT block → ZMQ publisher feeds web app spectrum panel | ⏸️ deferred |
| C13 | Keplerian propagator: closed-form lunar orbit, fixed-width, `noexcept` | |
| C14 | Python bindings: pybind11 wrappers for C1–C11, C13; C12 exposed via GNURadio's own Python bindings | |
| C15 | Web backend: Python FastAPI + WebSocket, calls bindings, streams state to frontend | ⏸️ deferred |
| C16 | Web frontend: Three.js Moon globe, satellite orbits, rover marker, FFT panel | ⏸️ deferred |

---

## Fit Check: R × C (Current vs Planned)

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
