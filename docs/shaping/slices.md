---
shaping: true
---

# LunaLink — AFS Reference Implementation — Slices

## Timeline

| Slice | Ends with | Status |
|-------|-----------|--------|
| V1 | AFS-I baseband signal generated and plotted (with correct chip mapping) | Done |
| V2 | Complete dual-channel AFS transmitter + architecture docs + SISICD draft + Spec Findings draft | In progress |
| V3 | Full codec: encode + decode + CRC-24 + LDPC pipeline + interleaver/deinterleaver, all 22 MSG types | Not started |
| V4 | Validation & Interop: test vectors, BER curves, SISICD final, Spec Findings final, cross-decode | Not started |
| V5 | PNT pipeline: N satellites → composite IQ → correlator → position fix | Not started |
| V6 | Demo: web app + optionally Pluto+ SDR | Deferred |

All demos below are target end-states for each slice unless explicitly marked as already delivered.

---

## V1 — AFS-I Signal Foundation

**Demo:** Python one-liner generates a Gold-2046 chip sequence for PRN 1, modulates a test
data symbol, and plots the chip sequence and its power spectrum. The spectrum shows the
expected 1.023 MHz null-to-null bandwidth. Tests pass at ≥90% coverage.

**Parts:**
| Part | Mechanism |
|------|-----------|
| C1 | Code loader: parse `docs/references/006_GoldCode2046hex210prns.txt` into `static constexpr uint8_t[2046]` chip arrays for all 210 PRNs |
| C2 (partial) | BPSK modulator: AFS-I only — chip × `int8_t` data symbol → `{−1, +1}` sample sequence at 1.023 Mcps |
| C14 (partial) | pybind11 binding for C1 + C2: `lunalink.afs.prn_code(prn_id)` → `np.ndarray`, `lunalink.afs.modulate_i(prn, data)` → `np.ndarray` |
| — | **Architecture:** replace `cpp/example*` with proper module layout: `cpp/include/lunalink/`, `cpp/signal/`, `cpp/codec/`, `cpp/rx/`; update CMakeLists.txt |
| — | **Compliance matrix skeleton:** enumerate every LSIS-nnn, LSIS-FID0-nnn, LSIS-TBC-nnnn, and LSIS-TBD-nnnn identifier from the spec (~50+ IDs); all statuses TBD initially; columns: ID / Requirement / Mandatory-Optional / Status / Notes |
| — | **Signal chain block diagram** (transmitter side) |
| — | **Spec Findings log:** start structured findings from V1 — each entry: spec section, exact quote, ambiguity, our interpretation, alternatives, recommendation |
| — | **V1 visual:** embed `demo/v1_plot.py` output (chip sequence + power spectrum) as PNG in Sphinx docs |

---

## V2 — Complete AFS Transmitter + Architecture Docs

**Target demo:** A single Python call generates a complete 12-second AFS baseband IQ frame for one
LNSP node (PRN 1, Subframe 1 with BCH-encoded sync + minimal SB1 payload, SB2–SB4
zero-padded). Output API shape is TBD (candidate: interleaved IQ pairs or `complex64` array).
Plotting shows: IQ constellation (BPSK),
power spectrum (dual-channel, I at 1.023 MHz bandwidth, Q at 5.115 MHz), and chip sequence
alignment between I and Q channels.

**Docs deliverable:** exhaustive compliance matrix (every LSIS-nnn/TBC/TBD identifier,
mandatory/optional/implemented), signal chain block diagram (transmit path end-to-end),
implementation architecture RST page in Sphinx, interface definitions for all public C++
APIs, SISICD draft (field layouts for 14 TBW message types), Spec Findings Report draft
(structured findings accumulated since V1), test vector format specification (published
early for cross-team adoption).

**Parts:**
| Part | Mechanism |
|------|-----------|
| C1 | Planned end-state: all three PRN table loaders (Gold-2046, Weil-10230, Weil-1500) |
| C2 | Planned end-state: BPSK(1) AFS-I + BPSK(5) AFS-Q modulator; chip mapping: logic 0 → +1, logic 1 → −1 (spec §2.3.3, Table 8) |
| C3 | Tiered code combiner: XOR primary ⊕ secondary ⊕ tertiary → composite AFS-Q chip sequence; default interim mapping applies to PRN 1-12, with explicit assignment API for other PRNs |
| C4 | IQ multiplexer: upsample I by 5 and output interleaved `int16_t` IQ pairs (`shape (10230,2)` in Python) with equal normalized digital amplitude; 5.115 MSPS/channel |
| C5 | BCH(51,8) encoder: 8-stage Fibonacci LFSR with polynomial `1+X^3+X^4+X^5+X^6+X^7+X^8` (Figure-8-matching; see ambiguity note on text `763`), 52-symbol output — 51 encoded bits each XOR'd with SB1 bit 0, bit 0 prepended |
| C8 (partial) | Frame builder: prepend 68-symbol sync pattern (`CC63F74536F49E04A`) + BCH-encoded SB1 (52 symbols) + zero-padded SB2–SB4 into 12 s frame |
| C14 | pybind11 bindings for C1–C5 primitives (`prn_code`, `weil10230_code`, `weil1500_code`, `modulate_i`, `modulate_q`, `multiplex_iq`, `bch_encode`) |
| — | Compliance matrix RST (all LSIS-001 through LSIS-xxx, mandatory/optional/implemented/TBD) |
| — | Signal chain block diagram (Sphinx `.. graphviz::` or PNG) |
| — | Architecture RST page: module layout, C++ API surface, design decisions |
| — | SISICD draft: field layouts for 14 TBW message types (R7.4) |
| — | Spec Findings Report draft: structured findings accumulated since V1 (R7.5) |
| — | Test vector format spec: document binary layout, metadata fields, checksums — publish early for cross-team adoption (R7.1) |
| — | Interop outreach: contact other competing teams, propose shared test vector format and cross-decode schedule |
| — | V2 visual: embed IQ constellation + dual-channel power spectrum in Sphinx docs |

---

## V3 — Full Navigation Message Codec

**Target demo:** End-to-end encode → decode round-trip for all 22 message types. The 8 spec-defined
types (G1, G2, G4, G5, G8, G22, G24, G30) are tested with full bit-level fidelity: every
field populated with representative values from the spec, round-tripped through the complete
TX→RX pipeline, and verified bit-perfect against known-answer vectors. The 14 SISICD-defined
(TBW) types are smoke-tested: encode → decode round-trip with zero-filled and boundary-value
payloads, verifying frame integrity (CRC-24 pass) and correct message type identification.
All tests apply full BCH + LDPC encoding + 60×98 block interleaving, then run the full RX
path: block deinterleaver → LDPC decode → CRC-24 check → BCH decode → message deserialise.
The LDPC table pipeline (`scripts/gen_ldpc_tables.py`) reads spec CSVs 003a–003j and
generates C++ static sparse arrays.

**Parts:**
| Part | Mechanism |
|------|-----------|
| C5 | Planned end-state: BCH(51,8) codec — encoder (from V2) + correlation-based decoder (256 hypotheses, per spec §2.4.2.1) |
| C6 | LDPC encoder: `p1 = B⁻¹·A·s mod 2`, `p2 = C·s + D·p1 mod 2`; filler bits appended (0 for SB2, 10 zeros for SB3/SB4); systematic puncturing: first z=240 (SB2) or z=176 (SB3/SB4) bits dropped; static arrays from `scripts/gen_ldpc_tables.py` reading 003a–003j CSVs |
| C7 | LDPC decoder: punctured positions restored as LLR=0 erasures; min-sum BP, 50 iterations (compile-time constant), static LLR buffers ≤70 KB; no heap |
| C8 | Frame builder/parser complete: **TX** — full SB2/SB3/SB4 LDPC encoding + 60×98 block interleaver + CRC-24 append + frame assembly. **RX** — sync detection, block deinterleaver (inverse 60×98), LDPC decode, CRC-24 check (G(X)=(1+X)·P(X), spec LSIS-FID0-467), BCH decode, message deserialise |
| C9 | Message serialiser: 8 fully defined per spec (G1, G2, G4, G5, G8, G22, G24, G30); remaining 14 TBW in V1.0 per SISICD-defined layouts; `[[nodiscard]]` encode/decode pairs |
| C14 | pybind11 bindings for C5–C9: `encode_frame(node_config, messages)`, `decode_frame(symbols)` → `DecodedFrame` |
| — | **V3 visual:** embed BER-vs-Eb/N₀ plot (SF2 + SF3 vs uncoded BPSK + Shannon limit) as PNG in Sphinx docs |

---

## V4 — Validation & Interoperability

**Target demo:** All competition M1+M2+M3 deliverables complete. Test vectors published: one complete
encoded frame per MSG type in a documented binary format suitable for cross-team exchange.
BER-vs-Eb/N₀ curves for SF2 and SF3 plotted against uncoded BPSK and Shannon limit. The
full RX pipeline (deinterleaver → LDPC decode → CRC-24 → BCH decode → message deserialise)
successfully cross-decodes our own test vectors. SISICD and Spec Findings Report finalised.

**Parts:**
| Part | Mechanism |
|------|-----------|
| — | Test vectors: one complete encoded frame per MSG type, stored as reference binary files in `tests/vectors/`; documented format spec (R7.1) |
| — | BER curve script (Python): sweep Eb/N₀ 0–10 dB, plot SF2 + SF3 vs uncoded BPSK + Shannon limit (R7.2) |
| — | Cross-decode validation: full RX pipeline round-trips all test vectors (R7.3) |
| — | SISICD final: complete field layouts for all 14 TBW message types (R7.4) |
| — | Spec Findings Report final: all ambiguities, errors, implementation choices documented (R7.5) |
| — | Parameter sensitivity sweeps: Eb/N₀, Doppler, code phase offset vs decode success rate (R7.6) |
| — | Link budget analysis: connect spec receiver sensitivity (−160 dBW min, −147 dBW max per LSIS-110) to operational Eb/N₀, predicted BER, and frame decode success rate; tabulated in Sphinx docs (R7.7) |
| — | Quality report: `task report` generates coverage summary (C++ gcovr + Python pytest-cov), clang-tidy clean confirmation, sanitizer pass log, and compliance matrix status — single-page evidence for reviewers |

---

## V5 — PNT Pipeline (should-have)

**Target demo:** A Python script simulates 6 LunaNet satellites in circular lunar orbit (100 km altitude,
evenly spaced in inclination). Each satellite broadcasts a unique PRN with a MSG-G4 (clock +
ephemeris) payload. The library generates the composite AFS-I IQ (6 signals summed + AWGN).
The correlator acquires all 6 PRNs, extracts pseudoranges, and the PNT solver outputs a
rover position fix on the lunar surface. The script prints the 3D position error vs ground truth.
Typical error: <1 m (no atmospheric corrections needed for Moon).

**Parts:**
| Part | Mechanism |
|------|-----------|
| C10 | Software correlator (AFS-I): PCPS acquisition (2048-pt FFT, 40 Doppler bins); DLL (early-minus-late envelope) + Costas PLL tracking; static buffers ~30 KB for N=6; runs on software-generated composite IQ |
| C11 | PNT solver: `Eigen::Matrix<double, Dynamic, 4>` bounded N≤12; iterative weighted least-squares (`ColPivHouseholderQR`); outputs ECEF-equivalent lunar position + clock bias |
| C13 | Keplerian propagator: closed-form lunar orbit position at epoch; `noexcept`, fixed-width; used by both transmitter (satellite positions for IQ generation) and receiver (navigation message parsing) |
| C14 | pybind11 bindings for C10–C13: `correlate(composite_iq, prn_list)` → `Pseudoranges`, `solve_pnt(pseudoranges, ephemerides)` → `PntFix` |
| — | Simulation script: 6-satellite scenario, composite IQ generation, full pipeline, position error plot |
| — | **V5 visual:** embed 3D position error plot (computed vs ground truth) as PNG in Sphinx docs |

---

## V6 — Demo (deferred)

**Target demo:** Web app + optionally Pluto+ SDR. `task demo` launches the web app: Moon globe
(Three.js) with satellite orbits, rover position, decoded message feed. If Pluto+ is
connected, `task demo-sdr` adds real RF transmission at 2492.028 MHz with live FFT spectrum.

**Parts:**
| Part | Mechanism |
|------|-----------|
| C12 | GNURadio OOT module (`gr-lunalink`): `afs_source` block wraps `lunalink_core` → `gr_complex` IQ; gr-iio Pluto+ TX/RX; ZMQ publisher feeds web app spectrum panel |
| C14 | Complete Python bindings: all C1–C13 exposed; typed stubs (`.pyi`) generated |
| C15 | FastAPI server: WebSocket endpoint streams JSON state at 1 Hz; `asyncio`-based simulation loop |
| C16 | Three.js frontend: Moon globe, satellite orbits, rover marker, message feed, FFT spectrum panel |
| — | `task demo` / `task demo-sdr` Taskfile entries |
| — | Docs: demo RST page, SDR integration page, hardware setup instructions |
