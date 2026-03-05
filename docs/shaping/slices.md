---
shaping: true
---

# LunaLink — AFS Reference Implementation — Slices

## Timeline

| Slice | Ends with |
|-------|-----------|
| V1 | AFS-I baseband signal generated and plotted (with correct chip mapping) |
| V2 | Complete dual-channel AFS transmitter + architecture docs + SISICD draft + Spec Findings draft |
| V3 | Full codec: encode + decode + CRC-24 + LDPC pipeline + interleaver/deinterleaver, all 22 MSG types |
| V4 | Validation & Interop: test vectors, BER curves, SISICD final, Spec Findings final, cross-decode |
| V5 | PNT pipeline: N satellites → composite IQ → correlator → position fix |
| V6 | Demo: web app + optionally Pluto+ SDR (deferred) |

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
| — | **Docs scaffold:** compliance matrix skeleton (all LSIS-00x IDs, status TBD), signal chain block diagram (transmitter side) |

---

## V2 — Complete AFS Transmitter + Architecture Docs

**Demo:** A single Python call generates a complete 12-second AFS baseband IQ frame for one
LNSP node (PRN 1, Subframe 1 with BCH-encoded sync + minimal SB1 payload, SB2–SB4
zero-padded). Output is a `complex64` numpy array. Plotting shows: IQ constellation (BPSK),
power spectrum (dual-channel, I at 1.023 MHz bandwidth, Q at 5.115 MHz), and chip sequence
alignment between I and Q channels.

**Docs deliverable:** compliance matrix (mandatory/optional for all LSIS spec items),
signal chain block diagram (transmit path end-to-end), implementation architecture RST page
in Sphinx, interface definitions for all public C++ APIs, SISICD draft (field layouts for
14 TBW message types), Spec Findings Report draft (ambiguities and choices found so far).

**Parts:**
| Part | Mechanism |
|------|-----------|
| C1 | Complete: all three PRN table loaders (Gold-2046, Weil-10230, Weil-1500) |
| C2 | Complete: BPSK(1) AFS-I + BPSK(5) AFS-Q modulator; chip mapping: logic 0 → +1, logic 1 → −1 (spec §2.3.3, Table 8) |
| C3 | Tiered code combiner: XOR primary ⊕ secondary ⊕ tertiary → composite AFS-Q chip sequence; secondary codes S0–S3 assigned per PRN |
| C4 | IQ multiplexer: scale I and Q to 50/50 power, sum into `int16_t` composite baseband buffer; ≥10.23 MSPS |
| C5 | BCH(51,8) encoder: polynomial 763 (octal), GF(2) division; 52-symbol output — 51 encoded bits each XOR'd with SB1 bit 0, bit 0 prepended; static LUT |
| C8 (partial) | Frame builder: prepend 68-symbol sync pattern (`CC63F74536F49E04A`) + BCH-encoded SB1 (52 symbols) + zero-padded SB2–SB4 into 12 s frame |
| C14 | pybind11 bindings for C1–C5, C8 partial: `generate_frame(prn, secondary_code, payload)` → `np.ndarray` |
| — | Compliance matrix RST (all LSIS-001 through LSIS-xxx, mandatory/optional/implemented/TBD) |
| — | Signal chain block diagram (Sphinx `.. graphviz::` or PNG) |
| — | Architecture RST page: module layout, C++ API surface, design decisions |
| — | SISICD draft: field layouts for 14 TBW message types (R7.4) |
| — | Spec Findings Report draft: ambiguities, errors, implementation choices (R7.5) |

---

## V3 — Full Navigation Message Codec

**Demo:** End-to-end encode → decode round-trip for all 22 message types. A Catch2 test
encodes each MSG type with representative field values, packs into SB2–SB4, applies full
BCH + LDPC encoding + 60×98 block interleaving, then runs the full RX path: block
deinterleaver → LDPC decode → CRC-24 check → BCH decode → message deserialise, and
verifies bit-perfect recovery. The LDPC table pipeline (`scripts/gen_ldpc_tables.py`)
reads spec CSVs 003a–003j and generates C++ static sparse arrays.

**Parts:**
| Part | Mechanism |
|------|-----------|
| C5 | Complete: BCH(51,8) codec — encoder (from V2) + correlation-based decoder (256 hypotheses, per spec §2.4.2.1) |
| C6 | LDPC encoder: `p1 = B⁻¹·A·s mod 2`, `p2 = C·s + D·p1 mod 2`; filler bits appended (0 for SB2, 10 zeros for SB3/SB4); systematic puncturing: first z=240 (SB2) or z=176 (SB3/SB4) bits dropped; static arrays from `scripts/gen_ldpc_tables.py` reading 003a–003j CSVs |
| C7 | LDPC decoder: punctured positions restored as LLR=0 erasures; min-sum BP, 50 iterations (compile-time constant), static LLR buffers ≤70 KB; no heap |
| C8 | Frame builder/parser complete: **TX** — full SB2/SB3/SB4 LDPC encoding + 60×98 block interleaver + CRC-24 append + frame assembly. **RX** — sync detection, block deinterleaver (inverse 60×98), LDPC decode, CRC-24 check (G(X)=(1+X)·P(X), spec LSIS-FID0-467), BCH decode, message deserialise |
| C9 | Message serialiser: 8 fully defined per spec (G1, G2, G4, G5, G8, G22, G24, G30); remaining 14 TBW in V1.0 per SISICD-defined layouts; `[[nodiscard]]` encode/decode pairs |
| C14 | pybind11 bindings for C5–C9: `encode_frame(node_config, messages)`, `decode_frame(symbols)` → `DecodedFrame` |

---

## V4 — Validation & Interoperability

**Demo:** All competition M1+M2+M3 deliverables complete. Test vectors published: one complete
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

---

## V5 — PNT Pipeline (should-have)

**Demo:** A Python script simulates 6 LunaNet satellites in circular lunar orbit (100 km altitude,
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

---

## V6 — Demo (deferred)

**Demo:** Web app + optionally Pluto+ SDR. `task demo` launches the web app: Moon globe
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
