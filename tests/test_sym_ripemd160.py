"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

import hashlib

import pytest

from cryptosym import SymBitVec, SymRIPEMD160, utils


class TestSymRIPEMD160:
    def test_sym_ripemd160_features(self):
        h = SymRIPEMD160(128)
        assert h.difficulty == 160
        assert h.difficulty == h.default_difficulty()
        assert h.num_input_bits == 128
        assert h.hash_name() == "RIPEMD160"

        rep = h.symbolic_representation()
        assert len(rep.input_indices) == 128
        assert len(rep.output_indices) == 160

        bv = SymBitVec(utils.random_bits(128))
        output = h.forward(bv)  # Call `forward` with SymBitVec
        assert isinstance(output, SymBitVec)
        assert len(output) == 160

        output = h()  # Call on random input bytes
        assert isinstance(output, bytes)
        assert len(output) == 160 // 8

        input_data = utils.random_bits(h.num_input_bits)
        output = h(input_data)  # Call on given input bytes
        assert isinstance(output, bytes)
        assert len(output) == 160 // 8

    def test_bad_number_of_input_bits(self):
        with pytest.raises(ValueError):
            _ = SymRIPEMD160(num_input_bits=31)

    def test_input_size_mismatch(self):
        h = SymRIPEMD160(32)
        with pytest.raises(ValueError):
            _ = h(hash_input=utils.random_bits(24))

    def test_set_readonly_properties(self):
        h = SymRIPEMD160(128)
        with pytest.raises(AttributeError):
            h.num_input_bits = 256
        with pytest.raises(AttributeError):
            h.difficulty = 100

    def test_against_hashlib(self, helpers):
        input_sizes = [0, 64, 256, 584, 1024, 2048, 4936]
        for inp_size in input_sizes:
            h = SymRIPEMD160(inp_size)
            input_bytes = utils.random_bits(inp_size)
            hlib = hashlib.new("ripemd160")
            hlib.update(input_bytes)
            expected = helpers.reverse_endianness(hlib.digest())
            assert h(input_bytes) == expected
