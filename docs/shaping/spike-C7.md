---
shaping: true
---

## C7 Spike: LDPC Min-Sum Belief Propagation — CLOSED ✅

### Findings

#### C7-Q1: Exact submatrix dimensions

**SF2 submatrices:**

| File | Submatrix | Rows × Cols |
|------|-----------|-------------|
| 004h_lunanet_sf2_ldpc_submatrix_a_mat.csv | A | 480 × 1200 |
| 004i_lunanet_sf2_ldpc_submatrix_b_inv_mat.csv | B⁻¹ | 480 × 480 |
| 004j_lunanet_sf2_ldpc_submatrix_b_mat.csv | B | 480 × 480 |
| 004f_lunanet_sf2_ldpc_submatrix_c_mat.csv | C | 4560 × 1200 |
| 004g_lunanet_sf2_ldpc_submatrix_d_mat.csv | D | 4560 × 480 |

**SF3 submatrices:**

| File | Submatrix | Rows × Cols |
|------|-----------|-------------|
| 004a_lunanet_sf3_ldpc_submatrix_a_mat.csv | A | 352 × 880 |
| 004b_lunanet_sf3_ldpc_submatrix_b_inv_mat.csv | B⁻¹ | 352 × 352 |
| 004c_lunanet_sf3_ldpc_submatrix_b_mat.csv | B | 352 × 352 |
| 004d_lunanet_sf3_ldpc_submatrix_c_mat.csv | C | 3344 × 880 |
| 004e_lunanet_sf3_ldpc_submatrix_d_mat.csv | D | 3344 × 352 |

#### C7-Q2: H matrix block structure

```
H = [ A  |  B  ]    ← rows: m1 (480 SF2, 352 SF3)
    [ C  |  D  ]    ← rows: m2 (4560 SF2, 3344 SF3)
```

B is square and invertible; B⁻¹ is pre-computed and provided in the CSV files. The full H has:
- SF2: (480 + 4560) × (1200 + 480) = **5040 × 1680**
- SF3: (352 + 3344) × (880 + 352) = **3696 × 1232**

#### C7-Q3: Code rate and block length

| | SF2 | SF3 |
|---|---|---|
| Code rate | 1/2 | 1/2 |
| Information bits (k) | 1200 (1176 data + 24 CRC) | 870 (846 data + 24 CRC) |
| Filler bits | 0 | 10 |
| Codeword length (n) | 2400 | 1740 |
| Lifting factor z | 120 (puncture 2z=240) | 88 (puncture 2z=176) |
| Transmitted systematic | 960 bits | 694 bits |
| Transmitted parity | 1440 bits | 1046 bits |

#### C7-Q4: Static buffer sizes

**Encoder** (GF(2) matrix-vector operations):
- SF2: s buffer 1200 bits, p1 buffer 480 bits, p2 buffer 4560 bits → ≤ 800 bytes total
- SF3: s buffer 880 bits, p1 buffer 352 bits, p2 buffer 3344 bits → ≤ 600 bytes total

**Decoder** (LLR message passing):
- Variable-node messages: n LLR values (floats) = 2400 × 4 = **9.6 KB** (SF2)
- Check-node messages: one per edge in H (number of 1s in H). From index CSVs: SF2 ~14k edges, SF3 ~11k edges → **56 KB / 44 KB** at float32
- Total per-instance decoder buffers: **~70 KB** (SF2), **~55 KB** (SF3) — static arrays, no heap

#### C7-Q5 & C7-Q6: Decoder iterations and spec parameters

The LSIS V1.0 spec **does not specify the number of decoding iterations** — left to the implementer. Standard practice for 5G NR LDPC: 20–50 iterations of min-sum BP. Min-sum (as opposed to sum-product) is preferred for fixed-point implementation. Iteration count is a compile-time constant → loop is fully bounded and deterministic.

#### C7-Q7: Systematic form

Yes — the code is systematic. The codeword is `[s; p1; p2]` before puncturing. Encoding does **not** require Gaussian elimination at runtime:

```
p1 = B⁻¹ · (A · s)    mod 2    ← B⁻¹ provided pre-computed
p2 = (C · s) + (D · p1)  mod 2
```

Both operations are sparse matrix × dense vector mod 2 — very fast. The 1s-only index CSVs (003_*) make this even more efficient: iterate over non-zero indices only.

### Conclusion

C7 is **fully resolved**. The LDPC encoder and decoder are both implementable in C++ with:
- Static arrays bounded to the known SF2/SF3 matrix dimensions
- No heap allocation
- Deterministic iteration count (compile-time constant)
- Encoder: two sparse GF(2) matrix-vector products using B⁻¹ (provided)
- Decoder: min-sum BP with ~50 iterations, ~70 KB of static LLR buffers

C6 (encoder) and C7 (decoder) both clear ⚠️. R2 and R7 pass.
