Flight-Heritage Coding Practices
=================================

This document explains the flight-heritage coding practices applied to this
codebase beyond standard software engineering practice. The goal is to
eliminate entire classes of defects using the same techniques employed in
flight software — fixed-width types, checked arithmetic, no heap allocation,
no exceptions — aligning with NASA, JAXA, and ESA mission-critical standards.

Threat Model
------------

Space software faces two categories of risk not present in typical
applications:

**Silent data corruption from C++ undefined behaviour (UB)**
  The C++ standard permits a compiler to assume UB never occurs and to
  optimise accordingly. When UB does occur at runtime (e.g. signed integer
  overflow, out-of-bounds access), the compiler's assumptions are violated
  and the resulting machine code may do anything — silently produce a wrong
  answer, corrupt adjacent memory, or eliminate a safety-critical check.
  There is no exception, no crash, no log entry.

**No operator to restart the mission**
  A ground application that enters a bad state can be killed and restarted
  in seconds. An orbital payload cannot. Every detected error must be handled
  explicitly, and every undetected error has permanent consequences.

The hardening measures below address both risks systematically.

C++20 Mission Hardening
-----------------------

Standard: C++20 (ISO/IEC 14882:2020)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The project utilizes C++20 to leverage modern safety features like ``std::span``,
``constinit``, and improved bitwise operations (``std::popcount``).

Fixed-Width Integer Types
~~~~~~~~~~~~~~~~~~~~~~~~~

All C++ functions use ``std::int32_t``, ``std::uint32_t``, etc. from ``<cstdint>``
rather than ``int``, ``unsigned``, or ``long``. This ensures numeric range 
predictability across radiation-hardened hardware targets (e.g. RISC-V, SPARC).

Semantic Type Integrity (PrnId)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To prevent "off-by-one" or "mixing-types" errors common in signal processing, 
the project uses a strong type ``PrnId`` for PRN identifiers. This prevents 
accidental arithmetic on IDs and ensures that a PRN 1 is never confused with 
a data bit 1 or a chip value 1.

Deterministic Initialization (constinit)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Critical global tables, such as the Sync Pattern and PRN packed codes, are
annotated with ``constinit``. This C++20 feature guarantees that these 
constants are initialized at compile-time (stored in ``.rodata``), eliminating
the "Static Initialization Order Fiasco" which can cause non-deterministic
boot behavior in embedded flight computers.

Bounds Safety (std::span and PrnCode)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Raw pointers are strictly avoided in public interfaces. The project uses
``std::span<T>`` to pass views of memory with integrated size checks. 
Furthermore, PRN resources are bundled into a ``PrnCode`` struct that 
couples the physical data span with its logical chip length, preventing 
buffer overreads.

Fail-Safe Execution (Zero-on-Error)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In alignment with JAXA and ESA standards, the signal chain follows a 
"Zero-on-Error" policy. Public functions pre-invalidate their output buffers
by zeroing them. If an error is detected during processing (e.g. BCH failure),
the buffer is cleared again before returning. This prevents the transmission 
of stale or partial navigation data.

Checked Integer Arithmetic
~~~~~~~~~~~~~~~~~~~~~~~~~~

The project follows an explicit-status pattern for input validation.
A representative API pattern is:

.. code-block:: cpp

    enum class ModulationStatus : std::uint8_t { kOk, kInvalidSymbol, kInvalidChipValue, kOutputTooSmall };
    [[nodiscard]] ModulationStatus modulate_bpsk_i(
        std::span<const uint8_t> chips,
        int8_t                   data_symbol,
        std::span<int8_t>        out) noexcept;

When validation fails, the C++ function returns a non-``kOk`` status.
The pybind11 layer converts these to Python ``ValueError`` exceptions.

Branchless Digital Signal Processing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Performance-critical paths (BPSK modulation, sample validation) are 
implemented using branchless arithmetic. This provides constant-time 
execution, improving performance predictability and reducing timing 
side-channel leakage.

``[[nodiscard]]``
~~~~~~~~~~~~~~~~~

Every function that returns a status or a computed value is annotated
``[[nodiscard]]``. Discarding a return value results in a compilation error.

Runtime Verification
--------------------

Sanitizers (ASan + UBSan)
~~~~~~~~~~~~~~~~~~~~~~~~~~

A dedicated CI job compiles the entire C++ test suite with
``-fsanitize=address,undefined``. This turns silent UB into hard, observable 
failures that blocks CI.

Run locally::

    task sanitize

Static Analysis
---------------

clang-tidy
~~~~~~~~~~

``clang-tidy`` enforces High-Integrity C++ rules (MISRA-aligned).
Checked families: ``bugprone-*``, ``cert-*``, ``cppcoreguidelines-*``, 
``hicpp-*``, ``fuchsia-*``.

Run locally::

    task tidy

Testing
-------

Coverage Requirements
~~~~~~~~~~~~~~~~~~~~~

- Python (``src/``): ≥ 90% statement **and** branch coverage (pytest-cov)
- C++ (``cpp/``): ≥ 90% line coverage (gcov/gcovr)

Standards Alignment
-------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Standard / Guideline
     - Alignment
   * - **ECSS-Q-ST-80C** (ESA)
     - Exhaustive error mapping; mandatory pre-initialization; formal pre/post contracts.
   * - **JAXA JRG-001**
     - Fail-safe output state; elimination of magic numbers; aliasing protection.
   * - **NASA Mission Hardening**
     - Bit-oriented safety; constinit determinism; branchless DSP logic.
   * - **MISRA C++:2023**
     - No heap allocation (Rule 21.6.1); no exceptions/RTTI; fixed-width types.
