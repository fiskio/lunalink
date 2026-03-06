"""LunaLink AFS subpackage — Augmented Forward Signal implementation."""

from lunalink.afs.signal import (
    EPOCHS_PER_FRAME,
    modulate_i,
    prn_code,
    tiered_code_epoch,
    weil1500_code,
    weil10230_code,
)

__all__ = [
    "prn_code",
    "weil10230_code",
    "weil1500_code",
    "modulate_i",
    "tiered_code_epoch",
    "EPOCHS_PER_FRAME",
]
