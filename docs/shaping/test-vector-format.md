---
shaping: true
---

# LunaLink — Test Vector Format Specification (R7.1)

## Purpose

Enable cross-team interoperability testing by publishing encoded AFS frames
alongside their ground truth message fields. Any team can decode our vectors
with their own receiver pipeline and compare results — without installing our
library.

## Design Principles

1. **Layered** — intermediate stages exposed so teams can binary-search for
   divergence points, not just compare final output
2. **Zero-dependency validation** — a standalone Python 3 script checks all
   vectors using only the standard library
3. **Spec-traceable** — every field links back to a spec table and section
4. **Versioned** — vectors are tagged and distributed via GitHub Releases,
   independently from the library version

## Signal Chain and Vector Layers

```
Message fields (JSON)              ← L0: ground truth
    ↓ serialize
Information bits                   ← L1: packed bit fields (hex in JSON)
    ↓ CRC-24 append
Protected bits                     ← L2: checksum (hex in JSON)
    ↓ LDPC encode + interleave
Encoded symbols (0/1)              ← L3: per-subframe binary files
    ↓ BCH(51,8) + sync prepend
Complete frame symbols (0/1)       ← L4: full frame binary file
    ↓ chip mapping + spreading + BPSK modulation
Baseband IQ (complex float32)      ← NOT shared (deterministic, ~490 MB/vector)
```

Test vectors cover **L0 through L4**. Everything below L4 (chip mapping,
spreading, modulation) is deterministic and trivial — a team that matches L4
will produce identical RF.

## Layer Details

| Layer | Content | Where | Size (typical) |
|-------|---------|-------|-----------------|
| L0 | Message field values with units, bit widths, scale factors, spec table refs | `vector.json` | ~2 KB |
| L1 | Serialized information bits (before FEC) | `vector.json` (hex string) | ~330 hex chars |
| L2 | CRC-24 value | `vector.json` (6 hex chars) | 6 chars |
| L3 | LDPC-encoded + interleaved symbols, concatenated SB2+SB3+SB4 | `subframes.bin` (1 byte/symbol, 0x00 or 0x01) | ~17 KB |
| L4 | Complete frame: sync + SB1 + SB2 + SB3 + SB4 | `frame.bin` (1 byte/symbol, 0x00 or 0x01) | ~17 KB |

**Why 1 byte per symbol (not packed bits)?** Avoids endianness and bit-ordering
ambiguity. Any language reads them trivially. File size is negligible
(~380 KB total for all 22 vectors).

## File Layout

```
vectors/
├── manifest.json                 # format version, spec ref, generation metadata
├── validate.py                   # standalone checker (Python 3 stdlib only)
├── G01_ephemeris/
│   ├── vector.json               # L0 + L1 + L2 + SHA-256 refs to binary files
│   ├── subframes.bin             # L3
│   └── frame.bin                 # L4
├── G02_almanac/
│   ├── vector.json
│   ├── subframes.bin
│   └── frame.bin
├── ...                           # one directory per MSG type
└── G30_utc/
    ├── vector.json
    ├── subframes.bin
    └── frame.bin
```

22 directories total (8 spec-defined + 14 SISICD-defined).

## manifest.json

```json
{
  "format_version": "1.0.0",
  "spec_reference": "LSIS V1.0",
  "generated_by": "lunalink",
  "generated_by_version": "0.2.0",
  "generated_by_commit": "abc1234",
  "generated_at": "2026-03-15T12:00:00Z",
  "vector_count": 22,
  "vectors": [
    "G01_ephemeris",
    "G02_almanac",
    "..."
  ]
}
```

## vector.json Schema

```json
{
  "format_version": "1.0.0",
  "spec_reference": "LSIS V1.0",
  "message_type": "G04",
  "message_name": "Clock & Ephemeris",
  "spec_section": "§2.5.4",
  "spec_status": "fully_defined",
  "prn": 1,
  "subframe": 2,
  "frame_id": 0,
  "toi": 42,

  "L0_fields": {
    "t_oc": {
      "value": 3600,
      "unit": "s",
      "bits": 16,
      "scale": "2^4",
      "spec_table": "Table 15"
    },
    "a_f0": {
      "value": 1.5e-6,
      "unit": "s",
      "bits": 25,
      "scale": "2^-34",
      "spec_table": "Table 15"
    }
  },

  "L1_info_bits_hex": "A3F7...0000",
  "L1_info_bits_length": 1320,
  "L1_filler_bits": 0,

  "L2_crc24_hex": "B7A3F1",

  "L3_subframes_sha256": "e3b0c44298fc...",
  "L3_subframes_sizes": [5880, 5880, 5880],

  "L4_frame_sha256": "a1b2c3d4e5f6...",
  "L4_frame_symbols": 17760
}
```

**`spec_status`** is `"fully_defined"` for the 8 spec-defined types (G1, G2,
G4, G5, G8, G22, G24, G30) or `"sisicd_defined"` for the 14 TBW types.

## validate.py

Zero-dependency (Python 3 stdlib only) standalone validation script:

- Parses `manifest.json`, iterates all vector directories
- Verifies SHA-256 checksums of `.bin` files match JSON declarations
- Checks binary file sizes match declared symbol counts
- Validates all required JSON fields are present
- Prints PASS/FAIL per vector, exits non-zero on any failure

Any team can run `python3 validate.py vectors/` without installing anything.

## GitHub Releases

Vectors are versioned and distributed independently from the library:

| Aspect | Detail |
|--------|--------|
| Tag format | `vectors-v1.0.0` |
| Asset | `lunalink-test-vectors-v1.0.0.tar.gz` |
| First release (V2 milestone) | 8 spec-defined types only — independently verifiable |
| Second release (V3 milestone) | All 22 types — includes SISICD-defined types |
| Release notes | Changelog, spec version, MSG types included, known limitations |

Versioning separately from the library allows re-releasing corrected vectors
without implying a library change.

## Integration Tests (CI)

The same vectors power our CI pipeline — four test suites:

| Test | What it proves |
|------|----------------|
| **Golden encode** | `encode(L0_fields)` produces output identical to `frame.bin` — guards against regressions |
| **Golden decode** | `decode(frame.bin)` recovers `L0_fields` bit-perfect — proves full RX pipeline |
| **Noisy decode** | `decode(frame.bin + AWGN @ Eb/N₀)` recovers fields — proves LDPC decoder robustness |
| **Cross-layer** | `encode(L0)` intermediate outputs match L1, L2, L3 — catches silent internal changes |

Golden tests use `frame.bin` as the single source of truth. If the encoder
changes, the test fails until vectors are deliberately regenerated with a
version bump.

## Generation Workflow

```
task vectors          # regenerate all vectors from current code
task vectors-check    # validate checksums (CI runs this)
task vectors-release  # tag + create GitHub release (manual, prompted)
```

## Size Budget

| Item | Per vector | All 22 vectors |
|------|-----------|-----------------|
| `vector.json` | ~2 KB | ~44 KB |
| `subframes.bin` | ~17 KB | ~374 KB |
| `frame.bin` | ~17 KB | ~391 KB |
| **Total** | ~36 KB | **~809 KB** |

Under 1 MB total. No compression, no git-lfs needed. Committed directly to the
repository.

**Note:** Noisy IQ vectors for BER testing (V4) are large (~490 MB each) and
are distributed only as GitHub Release assets, never committed to the repo.
