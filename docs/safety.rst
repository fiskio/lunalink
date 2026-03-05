Software Quality for Space
==========================

This document explains the additional hardening applied to this codebase
beyond standard software engineering practice. The goal is to eliminate
entire classes of defects that are acceptable risks in ground software but
are unacceptable when the code executes on a satellite.

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

C++ Code Hardening
------------------

Fixed-Width Integer Types
~~~~~~~~~~~~~~~~~~~~~~~~~

All C++ functions use ``std::int32_t``, ``std::uint32_t``, etc. from ``<cstdint>``
rather than ``int``, ``unsigned``, or ``long``. The C++ standard only
guarantees minimum widths for these types; on some space-qualified processors
(e.g. radiation-hardened RISC-V or SPARC derivatives) ``int`` can differ from
the 32-bit assumption common on desktop platforms.

Using fixed-width types makes the numeric range of every variable explicit
and portable.

Checked Integer Arithmetic
~~~~~~~~~~~~~~~~~~~~~~~~~~

Signed integer overflow is **undefined behaviour** in C++. The compiler may
silently produce the wrong answer, or — more dangerously — eliminate
surrounding code on the assumption that overflow never occurs. This is not a
theoretical risk: it is a real class of mission-critical bug.

Example: ``add(INT32_MAX, 1)`` with naïve ``a + b`` compiles and runs with no
warning, returning ``-2147483648`` silently.

Every arithmetic operation that can overflow uses ``__builtin_add_overflow``
(and its counterparts for subtraction and multiplication). This GCC/Clang
intrinsic maps to a single hardware instruction on all supported targets
(e.g. ``ADDO`` on SPARC, ``ADD`` + overflow flag test on x86/ARM) and has
zero runtime overhead compared with unchecked arithmetic.

The function signature encodes the overflow contract explicitly:

.. code-block:: cpp

    [[nodiscard]] bool add(std::int32_t a, std::int32_t b, std::int32_t& result) noexcept;
    //            ^^^^                                              ^^^^^^^
    //  caller MUST check the return value               no exception possible

When overflow is detected the C++ function returns ``false`` without touching
``result``. The pybind11 binding layer converts this to a Python
``OverflowError`` so Python callers receive a normal exception.

``[[nodiscard]]``
~~~~~~~~~~~~~~~~~

Every function that returns a status or a computed value is annotated
``[[nodiscard]]``. If a caller discards the return value — a common mistake
when a function is refactored from returning ``void`` to returning a status —
the compiler emits a warning that is treated as an error (``-Werror``).

``static_assert`` for Compile-Time Invariants
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Platform assumptions are verified at compile time so that a port to a new
target fails loudly at the build stage rather than silently at runtime:

.. code-block:: cpp

    static_assert(CHAR_BIT == 8,        "char must be 8 bits");
    static_assert(sizeof(std::int32_t) == 4, "std::int32_t must be exactly 32 bits");

Determinism and MSVC
~~~~~~~~~~~~~~~~~~~~

The core library is compiled with ``-fno-fast-math`` to guarantee strict IEEE 754 floating-point determinism across all targets and simulations. Floating point optimization shortcuts that sacrifice precision or reproducibility are explicitly blocked. Moreover, MSVC builds on Windows are blocked at the CMake level (``FATAL_ERROR``) because MSVC lacks the strict hardware-oriented zero-cost intrinsics (e.g. ``__builtin_add_overflow``) required for space-flight safety, ensuring developers cannot accidentally compile non-deterministic payloads.

``CMAKE_CXX_EXTENSIONS OFF``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Setting ``CMAKE_CXX_EXTENSIONS OFF`` forces ``-std=c++17`` rather than
``-std=gnu++17``. This disables GCC and Clang compiler extensions (e.g.
``__int128``, statement expressions) and ensures the codebase compiles under
the strict ISO C++17 standard on any conforming toolchain, including
cross-compilers for space-qualified processors.

Compiler Diagnostics
~~~~~~~~~~~~~~~~~~~~

The following warning flags are added beyond ``-Wall -Wextra -Wpedantic``,
and all warnings are treated as errors (``-Werror``):

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Flag
     - What it catches
   * - ``-Wconversion``
     - Implicit narrowing (e.g. ``std::int32_t`` ← ``int64_t``, ``int`` ← ``double``)
   * - ``-Wsign-conversion``
     - Signed/unsigned mismatch, a common source of range errors
   * - ``-Wshadow``
     - Inner variable silently hides outer variable
   * - ``-Wdouble-promotion``
     - ``float`` silently widened to ``double`` in expressions
   * - ``-Wnull-dereference``
     - Flow-sensitive null pointer dereference
   * - ``-Wundef``
     - Undefined macro used in ``#if`` silently becomes ``0``
   * - ``-Wcast-align``
     - Cast increases required alignment — crashes on strict-alignment targets
   * - ``-Wformat=2``
     - Format string vulnerabilities and type mismatches
   * - ``-Wold-style-cast``
     - C-style casting bypassing your type system (forces explicit casts)
   * - ``-Wimplicit-fallthrough``
     - Missing ``[[fallthrough]];`` in switch cases
   * - ``-Wlogical-op``, ``-Wduplicated-*``
     - Copy-paste logic errors in nested conditionals (GCC only)

Runtime Verification
--------------------

Sanitizers (ASan + UBSan)
~~~~~~~~~~~~~~~~~~~~~~~~~~

A dedicated CI job compiles the entire C++ test suite with
``-fsanitize=address,undefined`` and runs it. This enables two sanitizers:

**AddressSanitizer (ASan)**
  Instruments every memory access at runtime to detect:

  - Heap and stack buffer overflow
  - Use-after-free and use-after-return
  - Double-free, invalid-free

**UndefinedBehaviorSanitizer (UBSan)**
  Inserts runtime checks for every operation that has undefined behaviour
  under the C++ standard:

  - Signed integer overflow (the ``add`` function example above)
  - Null pointer dereference
  - Out-of-bounds array access
  - Invalid enum values, misaligned loads, invalid shifts

When a sanitizer violation is detected the program aborts immediately with a
precise diagnostic message (file, line, type of violation). This turns silent
UB into a hard, observable failure that blocks CI.

Run locally::

    task sanitize

Static Analysis
---------------

clang-tidy
~~~~~~~~~~

``clang-tidy`` performs pattern-based static analysis at the AST level,
catching bugs that escape both the compiler and the sanitizers because they
require reasoning about code patterns rather than runtime behaviour. The
configuration in ``.clang-tidy`` enables three check families:

**bugprone-\***
  Common bug patterns: narrowing conversions, signed char misuse, unsafe
  functions, integer overflow, suspicious comparisons, unused return values.

**cert-\***
  Checks derived from the CERT C++ Secure Coding Standard, covering integer
  safety (``cert-int*``), memory management (``cert-mem*``), and error
  handling (``cert-err*``).

**cppcoreguidelines-pro-\***
  Bounds safety (no pointer arithmetic, no unconstrained array access) and
  type safety (no C-style casts, no reinterpret_cast in flight code,
  no varargs).

**hicpp-\***
  Checks that enforce High-Integrity C++ rules, which closely mirror the
  MISRA C++ standard, ensuring a safe subset of the language.

**fuchsia-\***
  Strict C++ subset constraints (no default arguments, no static constructors)
  that align very well with aerospace guidelines.

All triggered checks are treated as errors (``WarningsAsErrors: "*"``).

The pybind11 binding file (``cpp/example.cpp``) is excluded because pybind11
template instantiations generate unavoidable noise with this strict check set.
The C++ core logic (``cpp/example_core.cpp``) and tests are fully checked.

Run locally (requires ``clang-tidy`` in ``PATH``)::

    task tidy

Testing
-------

Coverage Requirements
~~~~~~~~~~~~~~~~~~~~~

Both Python and C++ code have coverage gates that block CI if not met.

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Layer
     - Tool
     - Gate
   * - Python (``src/``)
     - pytest-cov
     - ≥ 90% statement **and** branch coverage
   * - C++ (``cpp/``)
     - gcov / gcovr
     - ≥ 90% line coverage

C++ coverage is collected by a separate ``coverage-cpp`` CI job that compiles
with ``--coverage`` (gcov instrumentation), runs the Catch2 tests, and uses
``gcovr`` to output HTML summaries and compute thresholds.

Boundary Value Testing
~~~~~~~~~~~~~~~~~~~~~~

The Catch2 test suite explicitly exercises the arithmetic boundaries of every
checked operation:

- Normal operation with positive, negative, zero, and mixed-sign operands
- Exact values at ``INT32_MAX`` and ``INT32_MIN`` (must not overflow)
- ``INT32_MAX + 1`` and ``INT32_MIN - 1`` (must detect overflow)

The Python test suite mirrors these cases and additionally verifies that
``OverflowError`` is raised correctly at the Python boundary.

Standards Alignment
-------------------

The hardening measures above align with the following guidelines and
standards:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Standard / Guideline
     - Alignment
   * - **ECSS-E-ST-40C** (ESA Software Engineering)
     - Statement and branch coverage measured and gated; cyclomatic complexity
       ≤ 10 enforced by ruff C90; all public APIs documented
   * - **JPL C++ Coding Standard** (Rule 2)
     - No dynamic memory allocation after initialisation; no heap usage in
       flight code (current codebase is fully static)
   * - **JPL C++ Coding Standard** (Rule 5)
     - Assertions (``static_assert``) used to verify all non-trivial
       preconditions at compile time
   * - **MISRA C++:2023** (Rule 21.6.1)
     - Dynamic memory (``new``, ``delete``, ``malloc``) not used. Enforced via ``cppcoreguidelines-no-malloc``.
   * - **MISRA C++:2023** (Exceptions and RTTI)
     - Core library (``lsis_afs_core``) compiled with ``-fno-exceptions`` and ``-fno-rtti``, ensuring deterministic control flow.
   * - **MISRA C++:2023** (integer safety)
     - Fixed-width types used throughout; checked arithmetic eliminates
       signed overflow UB; implicit conversions flagged by ``-Wconversion``
   * - **CERT INT30-C / INT32-C**
     - All integer operations that can overflow use ``__builtin_add_overflow``

What This Project Does Not Do
------------------------------

The following measures are common in the highest-criticality space software
but are not applied here:

**Formal verification**
  Model checking and theorem proving (e.g. Frama-C, SPARK/Ada) provide
  mathematical proofs of correctness for critical functions. This is
  appropriate for flight software with DO-178C / ECSS-Q-ST-80C certification
  requirements.

**MISRA C++ certification**
  Full MISRA compliance requires a commercial certified checker (e.g.
  Parasoft, LDRA, Polyspace) and a deviation management process. The measures
  above implement the most safety-relevant MISRA rules using free tooling.

**Worst-case execution time (WCET) analysis**
  Required for hard real-time systems. Tools such as AbsInt aiT or Rapita
  RVS are used for scheduling proofs on certified platforms.
