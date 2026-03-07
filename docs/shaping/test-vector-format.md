---
shaping: true
---

# LunaLink — Test Vector Format Specification (R7.1)

## Purpose

To ensure seamless cross-team interoperability, this project adopts the **Official LSIS-AFS Standardized Test Vector Format**.

## File Types & Structure

### 1. Spreading Codes (`codes_prn{N}.hex`)
Standardized hex export for PRN validation.
*   **Format:** ASCII text with `[SECTION]` headers.
*   **Sections:** `[GOLD_CODE]`, `[WEIL_PRIMARY]`, `[WEIL_TERTIARY]`, `[SECONDARY_S0...S3]`.
*   **Data:** Hexadecimal string (MSB-first).

### 2. Encoded Frames (`frame_{id}.bin`)
Binary symbols with a **64-byte metadata header**.
*   **Header (64 bytes):**
    *   Magic: `"LSISAFS\0"` (8 bytes)
    *   Version: `1` (4 bytes, uint32)
    *   Frame length: `6000` (4 bytes, uint32)
    *   PRN: `{N}` (4 bytes, uint32)
    *   Timestamp: Unix epoch (8 bytes, uint64)
    *   Reserved: 36 bytes (zero-filled)
*   **Data:** 6000 symbols (1 byte per symbol: `0x00` or `0x01`).

### 3. Baseband Signal (`signal_{prn}_{duration}s.iq`)
Binary I/Q samples with a **128-byte metadata header**.
*   **Header (128 bytes):**
    *   Magic: `"LSISIQ\0\0"` (8 bytes)
    *   Version: `1` (4 bytes, uint32)
    *   Sample rate: `float64` (8 bytes)
    *   Duration: `float64` seconds (8 bytes)
    *   PRN: `uint32` (4 bytes)
    *   Sample format: `"float32"` (16 bytes)
    *   Reserved: 80 bytes (zero-filled)
*   **Data:** Interleaved `float32` [I0, Q0, I1, Q1, ...].

### 4. Parsed Navigation Data (`parsed_{id}.json`)
JSON representation of all extracted fields.
*   **Required Top-Level:** `version`, `timestamp`, `frame_id`, `subframe1...4`, `time_of_transmission`.
*   **Format:** Matches the structure in `docs/references/interoperability.pdf`.

---

## Interoperability Test Levels

To ensure 100% compatibility, our validation follows the official 5-level protocol:

*   **Level 1: Code Gen** — Byte-by-byte match with Annex3 hex references.
*   **Level 2: Encoding** — Bit-identical sync patterns, BCH(51,8) symbols, and LDPC codewords across implementations for the 5 standard test messages.
*   **Level 3: Signal Gen** — Compatible I/Q files (Complex Float32 @ 10.23 MHz) that can be cross-decoded.
*   **Level 4: Decoding** — Cross-decode matrix: Our Decoder must decode Signal B (from another team) with BER < 10⁻⁵.
*   **Level 5: Message Parsing** — JSON export comparison of all extracted fields (WN, ITOW, ToT, etc.).

---

## Standardized Test Messages

All interoperability tests must use these five baseline payloads for Subframes 2-4:

| Scenario | Name | Description | Purpose |
|:---|:---|:---|:---|
| **MSG-1** | All Zeros | Baseline (all payload bits = 0) | Verify padding & CRC |
| **MSG-2** | All Ones | Inverse baseline (all payload bits = 1) | Verify scrambler/logic |
| **MSG-3** | Alternating | `0xAA...` (10101010...) | Verify interleaver pattern |
| **MSG-4** | Ephemeris | Known clock/ephemeris data from spec | Verify field serialization |
| **MSG-5** | Random | PRNG data with seed `0x12345678` | Stress test LDPC convergence |

---

## Baseline Verification Vectors (PRN 1)
...

These snippets from `interoperability.pdf` are used for Level 1 validation:
*   **Gold Code (first 32 chips):** `0x7A8B4C2D...`
*   **Weil Primary (first 32 chips):** `0x3F5E9A1B...`
*   **Weil Tertiary (first 32 chips):** `0x8C4D2E1F...`

