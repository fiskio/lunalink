---
shaping: true
---

# Progress Tracker

Single source of truth for what's done, what's next, and what's blocked.
Update this file when a part lands on `main`.

---

## Slices

| Slice | Gate | Status |
|-------|------|--------|
| V1 | AFS-I baseband signal generated and plotted | Done |
| V2 | Complete dual-channel AFS transmitter + architecture docs + SISICD draft | In progress |
| V3 | Full codec: encode + decode + CRC-24 + LDPC + interleaver, all 22 MSG types | Not started |
| V4 | Validation & Interop: test vectors, BER curves, SISICD final, Spec Findings final | Not started |
| V5 | PNT pipeline: N satellites -> composite IQ -> correlator -> position fix | Not started |
| V6 | Demo: web app + optionally Pluto+ SDR | Deferred |

---

## Parts

Legend: Done / PR open / In progress / Not started / Deferred

### Signal chain (transmitter)

| Part | Description | Slice | Status | PR / Notes |
|------|-------------|-------|--------|------------|
| C1 | Code loader (Gold-2046, Weil-10230, Weil-1500) | V1+V2 | Done | #2 merged |
| C2 | BPSK modulator — AFS-I BPSK(1) | V1 | Done | V1 commit |
| C2 | BPSK modulator — AFS-Q BPSK(5) | V2 | Done | C2-Q+C4 PR |
| C3 | Tiered code combiner | V2 | PR open | #3 — includes TieredCodeAssignment struct |
| C4 | IQ multiplexer (50/50 power, 5.115 MSPS) | V2 | Done | C2-Q+C4 PR |

### Navigation message (TX)

| Part | Description | Slice | Status | PR / Notes |
|------|-------------|-------|--------|------------|
| C5 | BCH(51,8) encoder | V2 | Not started | Generator poly 763 (octal), static LUT |
| C8 | Frame builder (partial: sync + BCH SB1 + zero-padded SB2-4) | V2 | Not started | 68-symbol sync + 52-symbol BCH + zero-pad |

### Navigation message (full codec)

| Part | Description | Slice | Status | PR / Notes |
|------|-------------|-------|--------|------------|
| C5 | BCH(51,8) decoder (correlation-based, 256 hypotheses) | V3 | Not started | |
| C6 | LDPC encoder (5G NR, SF2+SF3+SF4) | V3 | Not started | Spike closed (spike-C7.md) |
| C7 | LDPC decoder (min-sum BP, 50 iter, static buffers) | V3 | Not started | Spike closed (spike-C7.md) |
| C8 | Frame builder/parser complete (TX+RX, interleaver, CRC-24) | V3 | Not started | |
| C9 | Message serialiser (all 22 MSG types) | V3 | Not started | 8 spec-defined + 14 TBW |

### Receiver

| Part | Description | Slice | Status | PR / Notes |
|------|-------------|-------|--------|------------|
| C10 | Software correlator (PCPS acquisition + DLL/PLL tracking) | V5 | Not started | Spike closed (spike-C10.md) |
| C11 | PNT solver (Eigen, weighted least-squares) | V5 | Not started | |
| C13 | Keplerian propagator | V5 | Not started | |

### Infrastructure

| Part | Description | Slice | Status | PR / Notes |
|------|-------------|-------|--------|------------|
| C12 | GNURadio OOT module + Pluto+ SDR | V6 | Deferred | spike-C12.md open |
| C14 | pybind11 bindings | V1+ | Partial | C1+C2+C3 bound; grows with each part |
| C15 | Web backend (FastAPI) | V6 | Deferred | |
| C16 | Web frontend (Three.js) | V6 | Deferred | |

---

## Documentation deliverables

| Deliverable | Slice | Status | Notes |
|-------------|-------|--------|-------|
| Signal chain block diagram | V1 | Done | docs/signal/signal_chain.rst |
| Compliance matrix skeleton | V2 | Not started | All LSIS-nnn / TBC / TBD identifiers |
| Architecture RST page | V2 | Not started | Module layout, C++ API surface, design decisions |
| SISICD draft (14 TBW message types) | V2 | Not started | R7.4 |
| Spec Findings Report draft | V2 | Not started | R7.5 — accumulate since V1 |
| Test vector format spec | V2 | Started | docs/shaping/test-vector-format.md |
| BER curves (SF2+SF3 vs uncoded BPSK + Shannon) | V4 | Not started | R7.2 |
| SISICD final | V4 | Not started | R7.4 |
| Spec Findings Report final | V4 | Not started | R7.5 |
| Link budget analysis | V4 | Not started | R7.7 |
| Quality report (coverage, tidy, sanitizer) | V4 | Not started | |

---

## What's next

Priority order for V2 completion (after C3 merges):

1. **C5** — BCH(51,8) encoder
2. **C8 partial** — Frame builder (sync + SB1 + zero-padded SB2-4)
3. **Docs** — Compliance matrix, architecture page, SISICD draft, Spec Findings draft
