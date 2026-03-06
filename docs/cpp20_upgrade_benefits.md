# C++20 Upgrade Benefits for Lunalink

The Lunalink codebase has been upgraded from C++17 to C++20 to leverage modern language features that improve safety, robustness, and performance—critical requirements for space-flight applications.

## Key Advantages Exploited

### 1. Enhanced Safety with ``std::span``
We have replaced unsafe raw pointer and length pairs with ``std::span<T>``.
- **Bounds Safety**: ``std::span`` provides a safe view into contiguous memory, preventing accidental out-of-bounds access during signal processing.
- **API Clarity**: Functions now clearly communicate whether they expect a specific buffer size via fixed-extent spans where applicable.
- **Zero Overhead**: ``std::span`` is a lightweight abstraction that resolves to simple pointer arithmetic at runtime.

### 2. Deterministic Initialization with ``constinit``
Critical signal patterns (e.g. AFS Sync Pattern) and PRN tables are now declared ``constinit``.
- **Startup Safety**: Guarantees that these variables are initialized at compile-time, eliminating "Static Initialization Order Fiasco" risks during the boot sequence of flight computers.
- **Flash Memory Efficiency**: Ensures data is placed in read-only segments (``.rodata``), preventing accidental runtime corruption.

### 3. Bit-Oriented Performance with ``std::popcount``
The BCH(51,8) encoder utilizes C++20 bitwise operations for LFSR state management.
- **Hardware-Mapping**: ``std::popcount`` provides a high-performance way to calculate parity bits, mapping directly to efficient CPU instructions (e.g. ``POPCNT`` on x86 or ``VCNT`` on ARM).
- **Branchless Logic**: Enables arithmetic-based feedback loops that are faster and more predictable than branching logic.

### 4. Micro-Optimization with Branch Hints (``[[unlikely]]``)
Core signal processing paths (BCH encoding, framing, modulation) use C++20 attributes to hint the compiler about error conditions.
- **Instruction Cache Efficiency**: By marking error paths as ``[[unlikely]]``, the compiler keeps the "happy path" logic contiguous, improving cache hits.
- **Zero-Cost Validation**: Checks for invalid PRN IDs or buffer sizes are optimized for zero overhead when inputs are valid.

### 5. Modern Algorithms with ``std::ranges``
We have adopted C++20 Ranges for buffer operations like copying and filling.
- **Safer Syntax**: ``std::ranges::copy(src, dst.begin())`` is less error-prone than manually managing iterator pairs.
- **Improved Readability**: Range-based algorithms are more expressive and align with modern mission-critical standards.

## Impact on Space Flight Reliability

The move to C++20 is not just about using new syntax; it's about **hardening the software architecture**.
- **Deterministic Performance**: Optimized branching and modern abstractions ensure predictable timing for signal processing epochs.
- **Reduced Bug Surface**: Safer memory abstractions and strong types (like ``PrnId``) directly address the most common sources of flight software failures.
- **Traceability**: The modern code is easier to audit against technical specifications (like LSIS V1.0), as high-level intent is preserved in the implementation.
