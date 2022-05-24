"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from math import ceil

import pytest

from cryptosym import utils


def int2bytes(n: int, num_bits: int) -> bytes:
    num_bytes = int(ceil(num_bits / 8))
    return n.to_bytes(length=num_bytes, byteorder="big")


class TestUtils:
    def test_conversions(self):
        b = int2bytes(0b1101001100011101, 16)
        hex_str = "d31d"
        bin_str = "1101001100011101"
        assert utils.hexstr(raw_bytes=b) == hex_str
        assert utils.binstr(raw_bytes=b) == bin_str
        assert utils.hex2bits(hex_str=hex_str) == b

        deadbeef = int2bytes(0xDEADBEEF, 64)
        assert utils.hexstr(raw_bytes=deadbeef) == "00000000deadbeef"

        cheese = int2bytes(0x657365656863, 48)
        assert utils.str2bits(s="cheese") == cheese

    def test_bad_hex_string(self):
        with pytest.raises(ValueError):
            utils.hex2bits(hex_str="wxyz")

    def test_zero_bits(self):
        b = utils.zero_bits(n=32)
        assert b == int2bytes(0, num_bits=32)

    def test_random_bits(self):
        s = 42
        b1 = utils.random_bits(n=64, seed_value=s)
        b2 = utils.random_bits(n=64, seed_value=s)
        assert b1 == b2
        b3 = utils.random_bits(n=64)
        assert b1 != b3
        utils.seed(seed_value=s)
        b4 = utils.random_bits(n=64)
        assert b1 == b4
