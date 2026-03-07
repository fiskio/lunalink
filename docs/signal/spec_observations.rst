Specification Observations
==========================

This document records implementation assumptions and observations where the 
LunaNet Signal-In-Space Recommended Standard (LSIS V1.0, 29 January 2025) 
is ambiguous, interim, or contains editorial inconsistencies.

1. AFS-Q Code Assignment Beyond Interim Table 11
------------------------------------------------

*   **Context**: §2.3.5.4 and LSIS-260 state final matched-code assignment 
    and tertiary phasing are TBD (``LSIS-TBD-2001``). Table 11 provides 
    interim assignments only for LNSP Node IDs 1-12.
*   **LunaLink Assumption**: The default ``tiered_code_epoch(prn_id, ...)`` API 
    is restricted to PRNs 1-12. For PRNs outside this range, callers must 
    utilize the explicit assignment API (``tiered_code_epoch_assigned``) to 
    avoid silently extrapolating interim mappings.

2. AFS-Q Tertiary Phase Offset
------------------------------

*   **Context**: §2.3.5.4 notes tertiary phasing is TBD in a future release. 
    Table 11 interim values use phase offset ``0`` for all listed PRNs.
*   **LunaLink Assumption**: Default mappings use phase offset ``0``. The 
    library's internal engine supports non-zero phase offsets for forward 
    compatibility with future matched-code assignment updates.

3. Baseband IQ Power Scaling
----------------------------

*   **Context**: LSIS-103 defines 50/50 I/Q relative power. LSIS-130 includes 
    component power terms in the signal-generation equations.
*   **LunaLink Assumption**: The IQ Multiplexer (C4) implementation outputs 
    normalized baseband channels with unit amplitude (``{-1, +1}``). Absolute 
    power calibration and dynamic transmit power control are treated as 
    downstream stages (e.g., in the DAC/SDR frontend driver).

4. BCH(51,8) Generator Polynomial: Text vs. Diagram
---------------------------------------------------

*   **Context**: §2.4.2.1 states the generator polynomial is "763 (octal)" 
    (equivalent to :math:`x^8+x^7+x^6+x^5+x^4+x+1`). However, Figure 7 labels 
    the polynomial as :math:`1+X^3+X^4+X^5+X^6+X^7+X^8` (771 octal).
*   ** LunaLink Resolution**: The Figure 8 test vector (SB1=0x045 → 
    encoded=0x229f61dbb84a0) was used as the ground truth. A Fibonacci LFSR 
    utilizing the **Figure 7 polynomial** reproduces the test vector exactly. 
    The text value "763 octal" does not match the test vector.
*   **LunaLink Assumption**: The implementation uses the Figure 7 polynomial. 
    The text "763 (octal)" is treated as an editorial error in the standard.

5. Digital Container Format for Interleaved IQ
----------------------------------------------

*   **Context**: The LSIS defines signal generation equations but does not 
    prescribe an in-memory software container format for digital samples.
*   **LunaLink Assumption**: LunaLink outputs baseband as interleaved ``int16`` 
    pairs (``[I0, Q0, I1, Q1, ...]``). This preserves explicit I/Q structure, 
    matches common SDR/DMA interface expectations, and avoids ambiguity with 
    complex scalar representations.
