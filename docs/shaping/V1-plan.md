---
shaping: true
---

# V1 Plan — AFS-I Signal Foundation

## Demo

```python
import lunalink.afs as afs
import numpy as np
import matplotlib.pyplot as plt

chips   = afs.prn_code(1)           # Gold-2046, PRN 1 → ndarray shape (2046,) uint8
samples = afs.modulate_i(chips, 1)  # BPSK(1), data symbol +1 → ndarray shape (2046,) int8

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))
ax1.plot(samples[:100])
ax1.set_title("AFS-I chip sequence (first 100 chips)")

freqs = np.fft.fftshift(np.fft.fftfreq(len(samples), 1 / 1.023e6))
psd   = np.abs(np.fft.fftshift(np.fft.fft(samples.astype(float)))) ** 2
ax2.plot(freqs / 1e6, 10 * np.log10(psd + 1e-12))
ax2.set_xlabel("Frequency (MHz)")
ax2.set_title("Power spectrum — null-to-null 1.023 MHz")
plt.tight_layout()
plt.show()
```

The spectrum null-to-null bandwidth is 1.023 MHz. Tests pass at ≥90% coverage.

---

## PRN Table Format

`docs/references/006_GoldCode2046hex210prns.txt` is a Python-style list:

```
GoldCode2046 = [ \
"17590C193EB85C40...",   ← 512 hex chars = 256 bytes = 2048 bits
"130BFFDA4199FCFE...",
...                       210 lines total
]
```

Each 512-hex-char string encodes **2048 bits**, packed MSB-first. The first 2046 bits are
chips; the last 2 bits are zero-padding. Chips are {0, 1}; the modulator converts them to
{−1, +1}.

---

## File Changes

### Create

| Path | Purpose |
|------|---------|
| `scripts/gen_prn_table.py` | Reads `docs/references/006_GoldCode2046hex210prns.txt`, unpacks bits, emits `prn_table.hpp` + `prn_table.cpp` |
| `cpp/include/lunalink/signal/prn_table.hpp` | `extern const uint8_t kGoldPrns[210][2046];` — generated, committed |
| `cpp/signal/prn_table.cpp` | Defines `kGoldPrns` — generated, committed |
| `cpp/include/lunalink/signal/prn.hpp` | `gold_prn()` declaration |
| `cpp/signal/prn.cpp` | C1 implementation |
| `cpp/include/lunalink/signal/modulator.hpp` | `modulate_bpsk_i()` declaration |
| `cpp/signal/modulator.cpp` | C2 partial implementation |
| `cpp/bindings/afs_module.cpp` | `PYBIND11_MODULE(_afs, m)` — replaces `cpp/example.cpp` |
| `cpp/tests/test_prn.cpp` | Catch2 tests for C1 |
| `cpp/tests/test_modulator.cpp` | Catch2 tests for C2 |
| `src/lunalink/afs/signal.py` | Python wrappers with full numpy docstrings |
| `src/lunalink/afs/_afs.pyi` | Updated type stubs |
| `tests/test_afs.py` | pytest tests for Python bindings |
| `demo/v1_plot.py` | The demo script above |
| `docs/signal/compliance_matrix.rst` | Skeleton compliance matrix, all spec IDs, status TBD |
| `docs/signal/signal_chain.rst` | Transmitter-side signal chain diagram |

### Modify

| Path | Change |
|------|--------|
| `CMakeLists.txt` | Rename `lsis_afs_core` → `lunalink_core`; replace `example*` sources with `cpp/signal/` and `cpp/bindings/`; replace test executable; carry forward `-fno-exceptions -fno-rtti -fno-fast-math` on core lib |
| `.clang-tidy` | Update "Applied to" comment to reflect new file layout |
| `.github/workflows/ci.yml` | `tidy` job: replace hardcoded `cpp/example_core.cpp cpp/tests/test_example.cpp` with glob: `find cpp/ -name '*.cpp' ! -path '*/bindings/*' \| xargs clang-tidy -p build/tidy` |
| `pyproject.toml` | Add `numpy>=1.24` and `matplotlib>=3.7` to dev deps |
| `src/lunalink/afs/__init__.py` | Export `prn_code`, `modulate_i` from `signal.py` |
| `docs/index.rst` | Add `signal/compliance_matrix` and `signal/signal_chain` to toctree |

### Delete

| Path | Replaced by |
|------|------------|
| `cpp/example.cpp` | `cpp/bindings/afs_module.cpp` |
| `cpp/example_core.cpp` | `cpp/signal/prn.cpp` + `cpp/signal/modulator.cpp` |
| `cpp/include/example.hpp` | `cpp/include/lunalink/signal/prn.hpp` + `modulator.hpp` |
| `cpp/tests/test_example.cpp` | `cpp/tests/test_prn.cpp` + `cpp/tests/test_modulator.cpp` |
| `src/lunalink/afs/example.py` | `src/lunalink/afs/signal.py` |
| `tests/example_test.py` | `tests/test_afs.py` |

---

## Implementation

### Step 1 — PRN table generator

`scripts/gen_prn_table.py`:

1. Read `docs/references/006_GoldCode2046hex210prns.txt`
2. Extract the 210 quoted hex strings (strip `GoldCode2046 = [`, quotes, commas, `\`)
3. For each hex string: `bytes.fromhex(hex_str)` → 256 bytes; unpack MSB-first into
   `uint8_t[2046]` (drop the last 2 padding bits of byte 255)
4. Write `cpp/include/lunalink/signal/prn_table.hpp`:
   ```cpp
   // GENERATED — do not edit. Re-run scripts/gen_prn_table.py to regenerate.
   #pragma once
   #include <cstdint>
   namespace lunalink::signal {
   extern const uint8_t kGoldPrns[210][2046];
   } // namespace lunalink::signal
   ```
5. Write `cpp/signal/prn_table.cpp` with the full initialiser list.

Run once; commit both generated files. Do not add a CMake custom command (avoids
requiring Python at C++ build time; the table is stable reference data).

**Note on storage class:** `extern const` (not `constexpr`) in a separate `.cpp` avoids
embedding 430 KB of data in every translation unit that includes the header. The values are
fixed at build time via code generation — semantically identical to `constexpr`.

### Step 2 — C1: PRN loader

**`cpp/include/lunalink/signal/prn.hpp`:**
```cpp
#pragma once
#include <cstdint>
namespace lunalink::signal {
    inline constexpr uint8_t  kGoldPrnCount   = 210;
    inline constexpr uint16_t kGoldChipLength = 2046;

    // Returns pointer to the 2046-chip Gold-2046 sequence for prn_id (1-indexed).
    // Precondition: prn_id in [1, 210]. Validated by the binding layer; UB otherwise.
    [[nodiscard]] const uint8_t* gold_prn(uint8_t prn_id) noexcept;
} // namespace lunalink::signal
```

**`cpp/signal/prn.cpp`:**
```cpp
#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/prn_table.hpp"
namespace lunalink::signal {
    const uint8_t* gold_prn(uint8_t prn_id) noexcept {
        return kGoldPrns[prn_id - 1u];
    }
} // namespace lunalink::signal
```

### Step 3 — C2 (partial): BPSK modulator

**`cpp/include/lunalink/signal/modulator.hpp`:**
```cpp
#pragma once
#include <cstdint>
namespace lunalink::signal {
    // Modulate chip sequence with a BPSK data symbol (AFS-I channel, BPSK(1)).
    //   chips       – array of {0, 1}, length chip_count
    //   data_symbol – +1 or -1
    //   out         – caller-allocated, length >= chip_count
    // Chip mapping per spec §2.3.3, Table 8: logic 0 → +1, logic 1 → −1;
    // result multiplied by data_symbol.
    void modulate_bpsk_i(
        const uint8_t* chips,
        uint16_t       chip_count,
        int8_t         data_symbol,
        int8_t*        out
    ) noexcept;
} // namespace lunalink::signal
```

**`cpp/signal/modulator.cpp`:**
```cpp
#include "lunalink/signal/modulator.hpp"
namespace lunalink::signal {
    void modulate_bpsk_i(const uint8_t* chips, uint16_t chip_count,
                         int8_t data_symbol, int8_t* out) noexcept {
        for (uint16_t i = 0; i < chip_count; ++i) {
            // Per spec §2.3.3, Table 8: logic 0 → +1, logic 1 → −1
            out[i] = static_cast<int8_t>((chips[i] != 0u ? int8_t{-1} : int8_t{1})
                                         * data_symbol);
        }
    }
} // namespace lunalink::signal
```

### Step 4 — C14 (partial): pybind11 bindings

**`cpp/bindings/afs_module.cpp`:**
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "lunalink/signal/prn.hpp"
#include "lunalink/signal/modulator.hpp"

namespace py = pybind11;
using namespace lunalink::signal;

PYBIND11_MODULE(_afs, m) {
    m.doc() = "LunaLink AFS C++ extension module.";

    m.def("prn_code",
        [](uint8_t prn_id) -> py::array_t<uint8_t> {
            if (prn_id < 1 || prn_id > kGoldPrnCount)
                throw py::value_error("prn_id must be in [1, 210]");
            // Copy into a numpy-owned buffer — gold_prn() returns a pointer into
            // static storage; pybind11 must own the returned array.
            auto out = py::array_t<uint8_t>(kGoldChipLength);
            std::copy_n(gold_prn(prn_id), kGoldChipLength,
                        out.mutable_data());
            return out;
        },
        py::arg("prn_id"),
        "Return the Gold-2046 chip sequence for PRN prn_id (1-indexed)."
    );

    m.def("modulate_i",
        [](py::array_t<uint8_t, py::array::c_style> prn, int data_symbol)
                -> py::array_t<int8_t> {
            if (data_symbol != 1 && data_symbol != -1)
                throw py::value_error("data_symbol must be +1 or -1");
            auto r   = prn.request();
            if (r.ndim != 1)
                throw py::value_error("prn must be a 1-D array");
            auto out = py::array_t<int8_t>(r.shape[0]);
            modulate_bpsk_i(
                static_cast<const uint8_t*>(r.ptr),
                static_cast<uint16_t>(r.shape[0]),
                static_cast<int8_t>(data_symbol),
                out.mutable_data()
            );
            return out;
        },
        py::arg("prn"), py::arg("data_symbol"),
        "Modulate a chip sequence with a BPSK data symbol (AFS-I channel)."
    );
}
```

### Step 5 — Python wrapper

**`src/lunalink/afs/signal.py`** — thin wrapper so Sphinx autodoc sees numpy docstrings:
```python
from __future__ import annotations
import numpy as np
from lunalink.afs._afs import modulate_i as _modulate_i
from lunalink.afs._afs import prn_code as _prn_code

__all__ = ["prn_code", "modulate_i"]

def prn_code(prn_id: int) -> np.ndarray:
    """Return the Gold-2046 chip sequence for the given PRN (1-indexed).

    Parameters
    ----------
    prn_id : int
        PRN index in [1, 210].

    Returns
    -------
    np.ndarray
        Shape (2046,), dtype uint8, values in {0, 1}.

    Raises
    ------
    ValueError
        If prn_id is not in [1, 210].
    """
    return _prn_code(prn_id)

def modulate_i(prn: np.ndarray, data_symbol: int) -> np.ndarray:
    """Modulate a chip sequence with a BPSK data symbol (AFS-I channel).

    Implements the chip-to-sample mapping for the AFS-I channel per spec §2.3.3, Table 8:
    logic 0 → +1, logic 1 → −1, then multiplied by data_symbol.

    Parameters
    ----------
    prn : np.ndarray
        Chip sequence, shape (N,), dtype uint8, values in {0, 1}.
    data_symbol : int
        Data symbol, must be +1 or −1.

    Returns
    -------
    np.ndarray
        Shape (N,), dtype int8, values in {−1, +1}.

    Raises
    ------
    ValueError
        If data_symbol is not ±1, or prn is not 1-D.
    """
    return _modulate_i(prn, data_symbol)
```

**`src/lunalink/afs/__init__.py`** — add exports:
```python
from lunalink.afs.signal import modulate_i, prn_code

__all__ = ["prn_code", "modulate_i"]
```

### Step 6 — CMakeLists.txt

Replace the placeholder `lsis_afs_core` with `lunalink_core`:

```cmake
add_library(lunalink_core STATIC
    cpp/signal/prn_table.cpp   # generated
    cpp/signal/prn.cpp
    cpp/signal/modulator.cpp
)
target_include_directories(lunalink_core PUBLIC cpp/include)
set_target_properties(lunalink_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(lunalink_core PRIVATE ${LUNALINK_WARNING_FLAGS})
target_compile_options(lunalink_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-exceptions -fno-rtti -fno-fast-math>
)

# Python extension
pybind11_add_module(_afs cpp/bindings/afs_module.cpp)
target_link_libraries(_afs PRIVATE lunalink_core)
target_compile_options(_afs PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)
install(TARGETS _afs DESTINATION lunalink/afs)

# C++ tests
add_executable(test_lunalink
    cpp/tests/test_prn.cpp
    cpp/tests/test_modulator.cpp
)
target_link_libraries(test_lunalink PRIVATE lunalink_core Catch2::Catch2WithMain)
target_compile_options(test_lunalink PRIVATE ${LUNALINK_WARNING_FLAGS})
catch_discover_tests(test_lunalink)
```

Rename the warning flags variable from `LSIS_WARNING_FLAGS` to `LUNALINK_WARNING_FLAGS`
while editing.

### Step 7 — C++ tests

**`cpp/tests/test_prn.cpp`:**
```cpp
TEST_CASE("gold_prn PRN 1 has correct length and chip values") {
    const auto* chips = gold_prn(1);
    // All chips must be 0 or 1
    for (uint16_t i = 0; i < kGoldChipLength; ++i)
        REQUIRE((chips[i] == 0u || chips[i] == 1u));
}

TEST_CASE("gold_prn returns distinct sequences for different PRNs") {
    // PRN 1 and PRN 2 differ in at least one chip
    const auto* p1 = gold_prn(1);
    const auto* p2 = gold_prn(2);
    bool differ = false;
    for (uint16_t i = 0; i < kGoldChipLength; ++i)
        if (p1[i] != p2[i]) { differ = true; break; }
    REQUIRE(differ);
}

TEST_CASE("gold_prn PRN 1 first byte matches reference") {
    // First byte of PRN 1 hex is 0x17 = 0001 0111b → chips 0,0,0,1,0,1,1,1
    const uint8_t expected[8] = {0,0,0,1,0,1,1,1};
    const auto* chips = gold_prn(1);
    for (int i = 0; i < 8; ++i)
        REQUIRE(chips[i] == expected[i]);
}
```

**`cpp/tests/test_modulator.cpp`:**
```cpp
TEST_CASE("modulate_bpsk_i chip mapping with +1 symbol") {
    // Per spec §2.3.3, Table 8: logic 0 → +1, logic 1 → −1
    const uint8_t chips[4] = {0, 1, 0, 1};
    int8_t out[4]{};
    modulate_bpsk_i(chips, 4, 1, out);
    REQUIRE(out[0] == 1);  REQUIRE(out[1] == -1);
    REQUIRE(out[2] == 1);  REQUIRE(out[3] == -1);
}

TEST_CASE("modulate_bpsk_i chip mapping with -1 symbol") {
    const uint8_t chips[4] = {0, 1, 0, 1};
    int8_t out[4]{};
    modulate_bpsk_i(chips, 4, -1, out);
    REQUIRE(out[0] == -1); REQUIRE(out[1] == 1);
    REQUIRE(out[2] == -1); REQUIRE(out[3] == 1);
}

TEST_CASE("modulate_bpsk_i full PRN 1 all values in {-1, +1}") {
    std::array<int8_t, kGoldChipLength> out{};
    modulate_bpsk_i(gold_prn(1), kGoldChipLength, 1, out.data());
    for (auto v : out)
        REQUIRE((v == -1 || v == 1));
}
```

### Step 8 — Python tests

**`tests/test_afs.py`:**
```python
import numpy as np
import pytest
from lunalink.afs import prn_code, modulate_i

class TestPrnCode:
    def test_shape_and_dtype(self):
        chips = prn_code(1)
        assert chips.shape == (2046,)
        assert chips.dtype == np.uint8

    def test_chips_binary(self):
        chips = prn_code(1)
        assert set(chips.tolist()).issubset({0, 1})

    def test_all_210_prns_load(self):
        for prn_id in range(1, 211):
            assert prn_code(prn_id).shape == (2046,)

    def test_prns_are_distinct(self):
        assert not np.array_equal(prn_code(1), prn_code(2))

    def test_out_of_range_raises(self):
        with pytest.raises((ValueError, Exception)):
            prn_code(0)
        with pytest.raises((ValueError, Exception)):
            prn_code(211)


class TestModulateI:
    def test_shape_and_dtype(self):
        out = modulate_i(prn_code(1), 1)
        assert out.shape == (2046,)
        assert out.dtype == np.int8

    def test_values_binary(self):
        out = modulate_i(prn_code(1), 1)
        assert set(out.tolist()).issubset({-1, 1})

    def test_symbol_flip(self):
        chips = prn_code(1)
        assert np.array_equal(modulate_i(chips, -1), -modulate_i(chips, 1))

    def test_invalid_symbol_raises(self):
        with pytest.raises((ValueError, Exception)):
            modulate_i(prn_code(1), 0)
```

---

## Dependencies

Add to `pyproject.toml` dev group:
```toml
"numpy>=1.24",
"matplotlib>=3.7",
```

These are also needed at test time (numpy), so also add to the `test` group or keep
in `dev` and ensure CI installs `--group dev`.

---

## Docs Scaffold

**`docs/signal/compliance_matrix.rst`** — skeleton table, one row per spec requirement ID
from LSIS V1.0. Columns: ID | Requirement | Mandatory/Optional | Status. All statuses TBD
in V1; filled in progressively through V2–V4.

**`docs/signal/signal_chain.rst`** — transmitter-side block diagram using
`.. graphviz::`:
```
PRN tables → [C1 Code loader] → [C2 BPSK mod] → [C4 IQ mux] → baseband IQ
                                 [C3 Code combiner] ↗
```
Annotate with chip rates (1.023 Mcps AFS-I, 5.115 Mcps AFS-Q) and sample types
(`uint8_t` chips → `int8_t` samples → `int16_t` composite).

---

## Definition of Done

- [ ] `scripts/gen_prn_table.py` runs cleanly; `prn_table.hpp` and `prn_table.cpp` committed
- [ ] `task test-cpp` — all Catch2 tests pass
- [ ] `uv run pytest` — all tests pass, ≥90% coverage
- [ ] `uv run pyright src/` — 0 errors, 0 warnings
- [ ] `uv run ruff check src/ tests/` — 0 errors
- [ ] `uv run python demo/v1_plot.py` — chip sequence and power spectrum rendered
- [ ] Power spectrum null-to-null bandwidth visually confirms 1.023 MHz
- [ ] `task docs-build` — clean build, no warnings
