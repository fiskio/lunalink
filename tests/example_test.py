"""Unit tests for the example module."""

from lsis_afs.example import add, add_logged


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

    def test_add_logged(self) -> None:
        """Test that add_logged returns the same result as add."""
        assert add_logged(2, 3) == add(2, 3)
