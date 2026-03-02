"""Example module exposing C++ implementations via pybind11 bindings."""

import logging

from lsis_afs._example import add

logger = logging.getLogger(__name__)

__all__ = ["add", "add_logged"]


def add_logged(a: int, b: int) -> int:
    """Add two integers and log the operation.

    Parameters
    ----------
    a : int
        First integer.
    b : int
        Second integer.

    Returns
    -------
    int
        Sum of a and b.
    """
    result = add(a, b)
    logger.debug("add(%d, %d) = %d", a, b, result)
    return result
