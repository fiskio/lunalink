Space-Flight Software (FSW) Hardening Guide
===========================================

This document outlines the mission-critical software patterns implemented in 
LunaLink to satisfy the safety and reliability requirements of **NASA (JPL/Goddard)**, 
**ESA (ESTEC)**, and **JAXA (Tsukuba)** for Class A Flight Software.

Overview
--------

In the high-radiation environment of the Lunar surface, terrestrial "best practices" 
are insufficient. LunaLink utilizes a multi-layered defense strategy to mitigate 
**Single Event Upsets (SEU)**, ensure **Control-Flow Integrity**, and maintain 
**Absolute Determinism**.

Core Hardening Techniques
-------------------------

### 1. SEU-Resistant Enumerations (ESA/NASA)
Standard sequential enums (0, 1, 2...) are vulnerable to register bit-flips. 
LunaLink uses **Non-Contiguous Bit Patterns** with high Hamming distance for all 
status codes (e.g., ``BchStatus``, ``FrameStatus``).

*   **Pattern:** Codes like ``0x5A`` (01011010) and ``0xA5`` (10100101).
*   **Benefit:** A single-bit flip in a register will transform a valid status 
    into an "Invalid" state rather than a different, but valid, state.

### 2. Variable Mirroring / Soft-TMR (JAXA)
Critical state variables during long-running loops are "mirrored" to detect 
ALU or register file corruption during calculation.

*   **Implementation:** The ``min_dist`` variable is paired with ``min_dist_mir`` 
    (its bitwise inverse). 
*   **Verification:** At every loop iteration, the implementation verifies that 
    ``min_dist == ~min_dist_mir``.
*   **Benefit:** Detects transient hardware faults *during* the execution of 
    digital signal processing blocks.

### 3. Control-Flow Integrity (CFI) Verification (NASA/JAXA)
Radiation can cause the CPU's Program Counter or loop iterators to jump, 
leading to skipped logic or early loop exits.

*   **Implementation:** Every safety-critical loop implements an independent 
    terminal count verification.
*   **Verification:** After the 400-cycle BCH search, the code verifies 
    ``if (loop_count != kBchCodebookSize)``.
*   **Benefit:** Ensures that the entire information space was searched and 
    no radiation-induced "shortcuts" were taken.

### 4. Reciprocal Sanity Checks (NASA/ESA)
Even if the search loop completes, the result must be mathematically verified 
through a different execution path.

*   **Implementation:** The BCH decoder re-encodes the "winner" and compares 
    the Hamming distance of the re-encoded vector against the found minimum.
*   **Benefit:** Catches "Silent Data Corruption" where the logic returns 
    a result that is structurally valid but mathematically incorrect.

### 5. Radiation Masking (JAXA)
Codebook entries often use standard word sizes (64-bit) for 52-bit symbols.

*   **Implementation:** Explicit bit-masking (``& kCodewordMask``) is applied 
    to all memory fetches before bitwise operations.
*   **Benefit:** Prevents bit-flips in "unused" memory bits from polluting the 
    Hamming distance calculation.

### 6. Abort-Safety & Exception-Free Logic (ECSS/MISRA)
In Flight Software, a crash (abort) is often a mission-ending event.

*   **Policy:** The C++ core is compiled with ``-fno-exceptions`` and ``-fno-rtti``.
*   **Pattern:** Avoidance of STL traps like ``std::array::at()`` which trigger 
    ``std::terminate()``. Instead, we use explicit bounds-checked spans and 
    iterator safety, returning status codes for all failure states.

### 7. Stack Scrubbing & Information Security (NASA)
Prevents sensitive state (timing, crypto-init, telemetry) from leaking 
between tasks through uninitialized stack memory.

*   **Implementation:** All internal temporary buffers (e.g., ``verify_buf``) 
    are explicitly zeroed (scrubbed) using ``std::fill`` before the function 
    scope returns.

### 8. Strict Semantic Typing (ESA)
Prevents human-induced "Dimensional Errors" (e.g., the Mars Climate Orbiter failure).

*   **Implementation:** Use of ``enum class Fid`` and ``struct Toi`` wrappers 
    instead of raw ``uint8_t``. 
*   **Benefit:** The compiler prevents accidental swapping of parameters in 
    complex navigation signal chains.

HSI & Memory Optimization
-------------------------

*   **Cache-Line Alignment:** Codebooks are forced to ``alignas(64)`` to ensure 
    optimal burst reads and cache-line locality on ARM/LEON hardware.
*   **Fixed-Size Spans:** Implementation utilizes ``std::span<T, N>`` to 
    provide the compiler with the constant "geometry" of the signal buffers, 
    enabling formal proof of memory safety.
