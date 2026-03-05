---
shaping: true
---

## C10 Spike: Software Correlator (Acquisition + Tracking) — CLOSED ✅

### Findings

#### C10-Q1: PCPS acquisition search dimensions

AFS-I: Gold-2046, 1.023 Mcps, BPSK(1). FFT size: 2048 (next power of 2 above 2046).

Lunar orbiter Doppler at 2492 MHz: ±10 kHz range (corresponds to ±1.2 km/s LOS velocity —
generous upper bound). Doppler bin spacing: 500 Hz (half the symbol rate, standard practice).
Search space: **2048 code phase bins × 40 Doppler bins** per satellite.

PCPS procedure per Doppler bin:
1. FFT of received signal (2048-pt) — computed once, reused across all satellites
2. Pointwise conjugate multiply with pre-computed code replica FFT (one per PRN)
3. IFFT → magnitude-squared → peak search over 2048 code phase bins

#### C10-Q2: Sample rate and buffer size

Minimum sample rate: 2× chip rate = **2.046 MSPS**.
One code period: 1 ms = 2046 samples → FFT buffer: **2048 complex samples**.
At `complex<float>` (8 bytes each): **16 KB per code period** — stack-allocatable.

#### C10-Q3: DLL and PLL discriminators

| Loop | Discriminator | Formula |
|------|--------------|---------|
| DLL | Early-minus-late envelope | `(|E| − |L|) / (|E| + |L|)` |
| PLL | Costas loop | `atan2(Q_P, I_P)` |

Chip spacing for E/L: 0.5 chips. Loop bandwidths: DLL ~1 Hz, PLL ~15 Hz. Both adequate
for lunar orbiter dynamics (not hard real-time; update rate = 1 Hz at symbol rate / 500).

#### C10-Q4: CPU budget

**Acquisition** (one-shot per PRN, runs until lock ~100–500 code periods):
- Per code period: 1 receive FFT + N_sat × 40 Doppler bins × (multiply + IFFT)
- For N=6, 40 bins: 1 + 6×40×2 = 481 × 2048-pt FFTs per ms → heavy but time-limited
- Strategy: **pre-acquire all PRNs before demo starts** (runs offline, takes ~1–5 s)

**Tracking** (continuous, real-time):
- 3 correlators (E/P/L) × 2046 MACs × 6 satellites = **37K MACs/ms**
- At `float`, single core: trivially light — well under 0.1% CPU
- **No SIMD required**

#### C10-Q5: Software-simulated vs Pluto+ RX

The correlator runs **entirely on the software-generated composite IQ** (N satellite signals
summed + simulated AWGN channel). The Pluto+ RX is used **only** for the live FFT spectrum
display panel — it is not in the correlator signal path. This completely decouples demo
reliability from SDR hardware. If the Pluto+ fails on demo day, the PNT fix still works.

#### C10-Q6: Static buffer sizes per channel

| Buffer | Size per channel | N=6 total |
|--------|-----------------|-----------|
| E/P/L correlator accumulators | 3 × `complex<float>` = 24 B | 144 B |
| Code replica (one period) | 2048 × `int8_t` = 2 KB | 12 KB |
| Receive buffer (one period) | 2048 × `complex<float>` = 16 KB | shared, 16 KB |
| DLL/PLL state (filter taps) | ~200 B | 1.2 KB |
| Pseudorange history (ring, 10) | 10 × `double` = 80 B | 480 B |
| **Total** | | **~30 KB** |

All fixed-size, stack-allocatable, no heap.

#### C10-Q7: AFS-I only for demo

Target **AFS-I only** (Gold-2046, 1.023 Mcps, BPSK(1)). Reasons:
- AFS-Q tiered code tracking requires simultaneous lock on primary (10230-chip) +
  secondary (4-chip) + tertiary (1500-chip) codes — a substantially harder problem
- AFS-I is a canonical GNSS correlator: well-understood, deterministic, testable
- For ranging (pseudoranges → PNT fix), AFS-I is sufficient
- AFS-Q pilot channel is primarily for weak-signal acquisition — not needed for demo

### Conclusion

C10 is **fully resolved**. The software correlator is implementable in C++ with:
- PCPS acquisition: pre-run before demo, ~1–5 s, no real-time constraint
- Tracking: 37K MACs/ms for N=6 — trivial CPU load
- All buffers static, ~30 KB total for N=6 channels
- Runs on software-simulated IQ — independent of Pluto+ RX
- Target AFS-I channel only

C10 clears ⚠️. R6.3 (correlator → pseudoranges) passes.
