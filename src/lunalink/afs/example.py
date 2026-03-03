"""Example module exposing C++ implementations via pybind11 bindings."""

import logging

from lunalink.afs._afs import add as _add

logger = logging.getLogger(__name__)

__all__ = ["add", "add_logged"]


def add(a: int, b: int) -> int:
    """Add two 32-bit integers.

    The underlying C++ implementation uses checked arithmetic
    (``__builtin_add_overflow``) to detect signed integer overflow without
    invoking undefined behaviour.

    Parameters
    ----------
    a : int
        First operand. Must fit in a signed 32-bit integer.
    b : int
        Second operand. Must fit in a signed 32-bit integer.

    Returns
    -------
    int
        Sum of *a* and *b*.

    Raises
    ------
    OverflowError
        If *a* or *b* cannot be represented as ``int32_t``, or if their sum
        overflows a signed 32-bit integer.
    """
    return _add(a, b)


def add_logged(a: int, b: int) -> int:
    """Add two 32-bit integers and log the operation.

    Parameters
    ----------
    a : int
        First operand. Must fit in a signed 32-bit integer.
    b : int
        Second operand. Must fit in a signed 32-bit integer.

    Returns
    -------
    int
        Sum of *a* and *b*.

    Raises
    ------
    OverflowError
        If *a* or *b* cannot be represented as ``int32_t``, or if their sum
        overflows a signed 32-bit integer.
    """
    result = _add(a, b)
    logger.debug("add(%d, %d) = %d", a, b, result)
    return result
