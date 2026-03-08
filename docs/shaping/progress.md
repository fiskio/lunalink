---
shaping: true
---

# Progress Tracker — Gateway Mapping

Single source of truth for what's done, what's next, and what's blocked.
This implementation is tracked against the **8-Gateway Competition Structure**.

---

## Gateway 1: Spreading Code Generation
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD1.1** | Spreading Code Loader (Gold, Weil, Legendre) | **Implemented [Hardened]** | Packed storage; TMR IDs; CBIT checksums |
| **GD1.2** | Tiered Code Combiner (Matched Code) | **Implemented [Hardened]** | CheckedRange; CFI; Secure Scrubbing |

---

## Gateway 2: Forward Error Correction
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD2.1** | BCH(51,8) Encoder/Decoder | **Implemented [Hardened]** | ML Decoding; Soft-TMR; CFI |
| **GD2.2** | LDPC Encoder (5G NR SF2/3/4) | **Implemented [Hardened]** | CSR-hardened; ALU mirroring; CFI |
| **GD2.2** | LDPC Decoder (Min-Sum BP) | **Not started** | Spike-C7 closed |
| **GD2.3** | Block Interleaver/Deinterleaver (60x98) | **Not started** | Phase 2 Priority |
| **GD2.4** | CRC-24 Generator/Validator | **Not started** | Phase 2 Priority |

---

## Gateway 3: Message Framing
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD3.1** | Subframe 1-4 Builders | **Partial [Hardened]** | SB1 (BCH) done; SEU-resistant enums |
| **GD3.2** | Frame Assembler (12s duration) | **Not started** | Concatenation logic |

---

## Gateway 4: Baseband Signal Generation
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD4.1** | BPSK Modulator (AFS-I + AFS-Q) | **Implemented [Hardened]** | Branchless; CFI loops; Stack scrubbing |
| **GD4.2** | IQ Multiplexer (5.115 MSPS) | **Implemented [Hardened]** | Upsampling; Aliasing protection; WIP ticks |

---

## Gateway 5: Frame Sync & Decoding
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD5.1** | Software Correlator (Acquisition/Tracking) | **Not started** | Spike-C10 closed |
| **GD5.2** | Sync Pattern Detector | **Not started** | Cross-correlation |

---

## Gateway 6: Message Parsing
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD6.1** | Message Parsers (all 22 types) | **Not started** | 8 spec-defined + 14 SISICD |
| **GD6.2** | ToT Calculator | **Not started** | LSIS-720 formula |

---

## Gateway 7: Integration & Validation
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD7.1** | PNT Solver (Weighted Least-Squares) | **Not started** | Eigen-based |
| **GD7.2** | SISE Calculation (Pos/Vel Error) | **Not started** | 95th percentile analysis |
| **GD7.3** | Performance Benchmark Suite | **Not started** | NFR validation |
| **GD7.4** | Interoperability Test Report | **Not started** | Official template |

---

## Gateway 8: Documentation & Examples
| ID | Deliverable | Status | Notes |
|:---|:---|:---:|:---|
| **GD8.1** | API Reference (Sphinx/Doxygen) | **Partial** | Ongoing |
| **GD8.2** | Usage Examples (Notebooks) | **Not started** | |
| **GD8.3** | Final Compliance Matrix | **Partial** | docs/signal/compliance_matrix.rst |
| **GD8.4** | Spec Findings Report | **Partial** | Renamed from observations |

---

## What's Next (Phase 2 Priority)

1.  **GD2.3/2.4:** Implement the **Block Interleaver** and **CRC-24** (prerequisites for full framing).
2.  **GD2.1 Decoder:** Implement the **BCH(51,8) correlation-based decoder**.
3.  **GD2.2 LDPC:** Finalize the **LDPC table pipeline** and integration.
4.  **GD3 Framing:** Complete the **Subframe 2-4 Builders** to enable 12s frame generation.
