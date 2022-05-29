"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

import hashlib

import pytest

from cryptosym import SymBitVec, SymSHA512, utils


def reverse_endianness(data: bytes) -> bytes:
    data = bytearray(data)
    data.reverse()
    return bytes(data)


class TestSymSHA512:
    def test_sym_sha256_features(self):
        h = SymSHA512(128)
        assert h.difficulty == 80
        assert h.difficulty == h.default_difficulty()
        assert h.num_input_bits == 128
        assert h.hash_name() == "SHA512"

        rep = h.symbolic_representation()
        assert len(rep.input_indices) == 128
        assert len(rep.output_indices) == 512

        bv = SymBitVec(utils.random_bits(128))
        output = h.forward(bv)  # Call `forward` with SymBitVec
        assert isinstance(output, SymBitVec)
        assert len(output) == 512

        output = h()  # Call on random input bytes
        assert isinstance(output, bytes)
        assert len(output) == 512 // 8

        input_data = utils.random_bits(h.num_input_bits)
        output = h(input_data)  # Call on given input bytes
        assert isinstance(output, bytes)
        assert len(output) == 512 // 8

    def test_bad_number_of_input_bits(self):
        with pytest.raises(ValueError):
            _ = SymSHA512(num_input_bits=31)

    def test_input_size_mismatch(self):
        h = SymSHA512(32)
        with pytest.raises(ValueError):
            _ = h(hash_input=utils.random_bits(24))

    def test_against_hashlib(self):
        input_sizes = [0, 64, 256, 584, 1024, 2048, 4936]
        for inp_size in input_sizes:
            h = SymSHA512(inp_size)
            input_bytes = utils.random_bits(inp_size)
            hlib = hashlib.sha512()
            hlib.update(input_bytes)
            expected = reverse_endianness(hlib.digest())
            assert h(input_bytes) == expected
