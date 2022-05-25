"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from math import ceil


def int_to_little_endian_bytes(n: int, num_bits: int) -> bytes:
    num_bytes = int(ceil(num_bits / 8))
    return n.to_bytes(length=num_bytes, byteorder="little")


def reverse_endianness(data: bytes) -> bytes:
    data = bytearray(data)
    data.reverse()
    return bytes(data)
