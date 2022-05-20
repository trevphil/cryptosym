from math import ceil

import pytest

from cryptosym import SymBitVec


def int2bytes(n: int, num_bits: int) -> bytes:
    num_bytes = int(ceil(num_bits / 8))
    return n.to_bytes(length=num_bytes, byteorder="little")


class TestSymBitVec:
    def test_conversions(self):
        bits_a = int2bytes(0b1101001100011101, 16)
        bv = SymBitVec(bits=bits_a)
        hex_str = "d31d"
        bin_str = "1101001100011101"
        assert bv.bits() == bits_a
        assert int(bv) == 0b1101001100011101
        assert bv.bin() == bin_str
        assert bv.hex() == hex_str
        assert len(bv) == 16

        bits_b = int2bytes(0xDEADBEEF, 64)
        bv1 = SymBitVec(bits=bits_b)
        bv2 = SymBitVec(n=0xDEADBEEF, size=64)
        assert bv1.bits() == bits_b
        assert bv2.bits() == bits_b
        assert int(bv1) == 0xDEADBEEF
        assert int(bv2) == 0xDEADBEEF
        assert bv1.hex() == "00000000deadbeef"
        assert bv2.hex() == "00000000deadbeef"
        assert len(bv1) == 64
        assert len(bv2) == 64

    def test_basic_operators(self):
        bv1 = SymBitVec(0b110101, size=6)
        bv2 = SymBitVec(0b011101, size=6)
        assert int(reversed(bv1)) == 0b101011
        assert int(reversed(bv2)) == 0b101110
        assert int(bv1.rotr(2)) == 0b011101
        assert int(bv1.rotl(2)) == 0b010111
        assert int(bv1 >> 3) == 0b000110
        assert int(bv1 << 3) == 0b101000
        assert int(~bv1) == 0b001010
        assert int(bv1 ^ bv2) == 0b101000
        assert int(bv1 & bv2) == 0b010101
        assert int(bv1 | bv2) == 0b111101
        assert int(bv1 + bv2) == 0b010010

    def test_equality(self):
        a = SymBitVec(0b110101, size=6)
        b = SymBitVec(0b110101, size=6)
        c = SymBitVec(0b110101, size=7)
        d = SymBitVec(0b111111, size=6)
        assert a == b
        assert not (a != b)
        assert not (a == c)
        assert a != c
        assert not (a == d)
        assert a != d

    def test_resizing(self):
        bv = SymBitVec(0b110101, size=6)
        bv_bigger = bv.resize(10)
        assert len(bv_bigger) == 10
        assert int(bv_bigger) == 0b0000110101
        bv_smaller = bv.resize(2)
        assert len(bv_smaller) == 2
        assert int(bv_smaller) == 0b01

    def test_concat(self):
        bv1 = SymBitVec(0b110101, size=6)
        bv2 = SymBitVec(0b011101, size=6)
        bv12 = bv1.concat(bv2)
        assert len(bv12) == 12
        assert int(bv12) == (0b011101 << 6) + 0b110101

    def test_indexing(self):
        bv = SymBitVec(0b1101, size=4)
        assert bv[0] == 1
        assert bv[1] == 0
        assert bv[2] == 1
        assert bv[3] == 1
        with pytest.raises(IndexError):
            _ = bv[4]
        assert bv[-1] == 1
        assert bv[-2] == 1
        assert bv[-3] == 0
        assert bv[-4] == 1
        with pytest.raises(IndexError):
            _ = bv[-5]

    def test_slicing(self):
        bv = SymBitVec(0b110101, size=6)
        assert len(bv[1:5]) == 4
        assert int(bv[1:5]) == 0b1010
        assert len(bv[1:-1]) == 4
        assert int(bv[1:-1]) == 0b1010
        assert int(bv[1:6]) == 0b11010
        assert len(bv[1:100]) == 5
        assert int(bv[1:100]) == 0b11010

    def test_bad_slicing(self):
        bv = SymBitVec(0b11111111, size=8)
        with pytest.raises(IndexError):
            _ = bv[4:2]
        with pytest.raises(IndexError):
            _ = bv[::2]
        with pytest.raises(IndexError):
            _ = bv[::-1]

    def test_iterating(self):
        bv = SymBitVec(0b110101, size=6)
        for i, val in enumerate(bv):
            assert val == ((0b110101 >> i) & 1)
        assert any(bv)
        assert not all(bv)
        assert all(SymBitVec(0b111111, size=6))
        assert not any(SymBitVec(0, size=32))

    def test_addition_with_zero(self):
        t0 = SymBitVec(0, size=32)
        t1 = SymBitVec(0b11010100010010100110100011100000, size=32)
        summed = t0 + t1
        assert int(t1) == int(summed)

    def test_three_way_xor(self):
        a = SymBitVec(0b11010101, size=8)
        b = SymBitVec(0b10001001, size=8)
        c = SymBitVec(0b01011111, size=8)
        assert int(SymBitVec.xor3(a, b, c)) == 0b00000011

    def test_majority3(self):
        a = SymBitVec(0b11010101, size=8)
        b = SymBitVec(0b10001001, size=8)
        c = SymBitVec(0b01011111, size=8)
        assert int(SymBitVec.maj3(a, b, c)) == 0b11011101

    def test_incompatible_sizes(self):
        a = SymBitVec(0b11111111, size=8)
        b = SymBitVec(0b01111111, size=7)
        c = SymBitVec(0b10110100, size=8)

        with pytest.raises(ValueError):
            _ = a & b
        with pytest.raises(ValueError):
            _ = a | b
        with pytest.raises(ValueError):
            _ = a ^ b
        with pytest.raises(ValueError):
            _ = a + b
        with pytest.raises(ValueError):
            _ = SymBitVec.maj3(a, b, c)
        with pytest.raises(ValueError):
            _ = SymBitVec.xor3(a, b, c)

    def test_rotate_right_by_large_number(self):
        a = SymBitVec(0b11111011, size=8)

        b = a.rotr(8)
        assert b[1] is True
        assert b[2] is False
        assert b[3] is True

        b = a.rotr(16)
        assert b[1] is True
        assert b[2] is False
        assert b[3] is True

        b = a.rotr(1)
        assert b[0] is True
        assert b[1] is False
        assert b[2] is True

        b = a.rotr(9)
        assert b[0] is True
        assert b[1] is False
        assert b[2] is True

    def test_rotate_left_by_large_number(self):
        a = SymBitVec(0b11111101, size=8)

        b = a.rotl(8)
        assert b[0] is True
        assert b[1] is False
        assert b[2] is True

        b = a.rotl(16)
        assert b[0] is True
        assert b[1] is False
        assert b[2] is True

        b = a.rotl(1)
        assert b[1] is True
        assert b[2] is False
        assert b[3] is True

        b = a.rotl(9)
        assert b[1] is True
        assert b[2] is False
        assert b[3] is True

    def test_left_shift(self):
        a = SymBitVec(0b11111101, size=8)

        b = a << 1
        assert len(b) == 8
        assert int(b) == 0b11111010

        b = a << 100
        assert len(b) == 8
        assert int(b) == 0

    def test_right_shift(self):
        a = SymBitVec(0b11111101, size=8)

        b = a >> 1
        assert len(b) == 8
        assert int(b) == 0b01111110

        b = a >> 100
        assert len(b) == 8
        assert int(b) == 0
