---
shaping: true
---

# Progress Tracker — Gateway Mapping

Single source of truth for what's done, what's next, and what's blocked.
This implementation is tracked against the **8-Gateway Competition Structure**.

---

## Gateway Mapping

| Gateway | Deliverable | Status | Parts Involved |
|:---:|:---|:---:|:---|
| **G1** | Spreading Code Generation | **Done** | C1, C3 |
| **G2** | FEC Encoding & Decoding | **In progress** | C5, C6, C7 |
| **G3** | Message Framing | **In progress** | C8 |
| **G4** | Baseband Signal Generation | **Done** | C2, C4 |
| **G5** | Frame Sync & Decoding | **Not started** | C10 (Spike done) |
| **G6** | Message Parsing | **Not started** | C9 |
| **G7** | Integration & Validation | **Not started** | — |
| **G8** | Documentation | **Partial** | — |

---

## Implementation Parts (C-Numbers)

Legend: Done / PR open / In progress / Not started / Deferred

### Gateway 1 & 4: Spreading & Signal Gen
| Part | Description | Status | Gateway | Notes |
|:---|:---|:---:|:---:|:---|
| C1 | Code loader (Gold, Weil, Legendre) | Done | G1 | #2 merged |
| C2 | BPSK modulator (AFS-I + AFS-Q) | Done | G4 | V1+V2 commit |
| C3 | Tiered code combiner | Done | G1 | Secondary code mapping (Table 10) |
| C4 | IQ multiplexer (5.115 MSPS) | Done | G4 | Chunked generation |

### Gateway 2: Forward Error Correction
| Part | Description | Status | Gateway | Notes |
|:---|:---|:---:|:---:|:---|
| C5 | BCH(51,8) Encoder | Done | G2 | Polynomial 763₈ (Figure 8 verified) |
| C5 | BCH(51,8) Decoder | In progress | G2 | Correlation-based (256 hypotheses) |
| C6 | LDPC Encoder (5G NR SF2/3/4) | Not started | G2 | Spike-C7 closed |
| C7 | LDPC Decoder (Min-Sum BP) | Not started | G2 | Spike-C7 closed |
| — | Block Interleaver/Deinterleaver | Not started | G2 | 60x98 matrix |
| — | CRC-24 Generator/Validator | Not started | G2 | G(X)=(1+X)·P(X) |

### Gateway 3: Message Framing
| Part | Description | Status | Gateway | Notes |
|:---|:---|:---:|:---:|:---|
| C8 | Frame Builder (TX) | Partial | G3 | Sync + BCH SB1 done |
| — | Subframe 2/3/4 Builders | Not started | G3 | LDPC + Interleaver required |
| — | Frame Assembler | Not started | G3 | 12s frame concatenation |

### Gateway 5 & 6: RX Pipeline
| Part | Description | Status | Gateway | Notes |
|:---|:---|:---:|:---:|:---|
| C10 | Software Correlator | Not started | G5 | Spike-C10 closed |
| — | Sync Pattern Detector | Not started | G5 | Cross-correlation |
| C9 | Message Parsers (all 22 types) | Not started | G6 | 8 spec-defined + 14 SISICD |
| — | ToT Calculator | Not started | G6 | LSIS-720 formula |

### Gateway 7: Integration, Performance & Utilities
| Part | Description | Status | Gateway | Notes |
|:---|:---|:---:|:---:|:---|
| C11 | PNT Solver (pseudoranges -> fix) | Not started | G7 | Eigen-based |
| — | SISE Calculation (Pos/Vel Error) | Not started | G7 | 95th percentile analysis |
| — | Link Budget Analysis | Not started | G7 | -160 dBW sensitivity |
| — | Final Compliance Report | Not started | G7 | Gateway 1-8 evidence |

---

## Documentation & Submission Deliverables

| Deliverable | Gateway | Status | Notes |
|:---|:---:|:---:|:---|
| Signal chain block diagram | G8 | Done | docs/signal/signal_chain.rst |
| Compliance Matrix (G1-G8) | G8 | Partial | docs/signal/compliance_matrix.rst |
| Architecture Description | G8 | Done | docs/signal/architecture.rst |
| SISICD (TBW field layouts) | G8 | Done | docs/signal/sisicd.rst |
| Spec Findings Report | G8 | Partial | docs/signal/spec_findings_report.rst |
| Test Vector Suite (codes, frames, signals) | G7 | Partial | Standardized binary format |
| **Interoperability Test Report** | G7 | Not started | Official 7-section template |
| **Performance Report** | G7 | Not started | Throughput/Latency analysis |
| **Compliance Checklist** | G7 | Not started | Final success metric audit |
| BER Performance Curves | G7 | Not started | Eb/N0 sweeps for SF2/3 |
| API Usage Examples (Notebooks) | G8 | Not started | G8 success criteria |

---

## What's Next (Phase 2 Priority)

1.  **G2 Core:** Implement the **Block Interleaver** and **CRC-24** (prerequisites for full framing).
2.  **G2 Decoder:** Implement the **BCH(51,8) correlation-based decoder**.
3.  **G2 LDPC:** Implement the **LDPC table pipeline** (`scripts/gen_ldpc_tables.py`) and **Encoder** (C6).
4.  **G3 Framing:** Complete the **Subframe 2-4 Builders** to enable 12s frame generation.
