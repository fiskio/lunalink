"""Unit tests for the example module."""

import pytest

from lunalink.afs.example import add, add_logged

INT32_MAX = 2**31 - 1
INT32_MIN = -(2**31)


class TestAdd:
    """Tests for the add function."""

    def test_add_positive(self) -> None:
        """Test adding two positive integers."""
        assert add(2, 3) == 5

    def test_add_negative(self) -> None:
        """Test adding two negative integers."""
        assert add(-1, -2) == -3

    def test_add_zero(self) -> None:
        """Test adding zeros."""
        assert add(0, 0) == 0

    def test_add_mixed(self) -> None:
        """Test adding a positive and a negative integer."""
        assert add(5, -3) == 2

    def test_add_at_boundaries(self) -> None:
        """Test addition at the exact int32_t boundary values."""
        assert add(INT32_MAX, 0) == INT32_MAX
        assert add(INT32_MIN, 0) == INT32_MIN
        assert add(INT32_MAX, -1) == INT32_MAX - 1
        assert add(INT32_MIN, 1) == INT32_MIN + 1

    def test_add_positive_overflow(self) -> None:
        """Test that positive overflow raises OverflowError."""
        with pytest.raises(OverflowError):
            add(INT32_MAX, 1)

    def test_add_negative_overflow(self) -> None:
        """Test that negative overflow raises OverflowError."""
        with pytest.raises(OverflowError):
            add(INT32_MIN, -1)

    def test_add_logged(self) -> None:
        """Test that add_logged returns the same result as add."""
        assert add_logged(2, 3) == add(2, 3)

    def test_add_logged_overflow(self) -> None:
        """Test that add_logged propagates OverflowError."""
        with pytest.raises(OverflowError):
            add_logged(INT32_MAX, 1)
