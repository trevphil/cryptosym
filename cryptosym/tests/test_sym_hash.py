"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

import pytest

from cryptosym import SymBitVec, SymHash, SymRepresentation, utils


class MockHashCustomConstructorNoSuper(SymHash):
    def __init__(self, name: str):
        # SymHash.__init__(self, ...) is not called <--- WRONG!
        self.name = name

    def default_difficulty(self) -> int:
        return 64

    def hash_name(self) -> str:
        return self.name

    def forward(self, hash_input: SymBitVec) -> SymBitVec:
        return hash_input ^ reversed(hash_input)


class MockHashCustomConstructor(SymHash):
    def __init__(self, name: str):
        SymHash.__init__(self, num_input_bits=512)  # GOOD!
        self.name = name

    def default_difficulty(self) -> int:
        return 64

    def hash_name(self) -> str:
        return self.name

    def forward(self, hash_input: SymBitVec) -> SymBitVec:
        return hash_input ^ reversed(hash_input)


class MockHashNoCustomConstructor(SymHash):
    def default_difficulty(self) -> int:
        return 64

    def hash_name(self) -> str:
        return "no_custom_constructor"

    def forward(self, hash_input: SymBitVec) -> SymBitVec:
        return hash_input ^ reversed(hash_input)


class TestSymHash:
    def test_base_class(self):
        h = SymHash(num_input_bits=64, difficulty=1)
        with pytest.raises(RuntimeError):
            _ = h.default_difficulty()
        with pytest.raises(RuntimeError):
            _ = h.hash_name()
        with pytest.raises(RuntimeError):
            bv = SymBitVec(0xDEADBEEF, size=64, unknown=True)
            _ = h.forward(hash_input=bv)
        with pytest.raises(RuntimeError):
            input_data = utils.random_bits(64)
            _ = h(hash_input=input_data)
        with pytest.raises(RuntimeError):
            _ = h()

    def test_child_class_custom_constructor(self):
        with pytest.raises(TypeError):
            _ = MockHashCustomConstructorNoSuper(name="crash")
        _ = MockHashCustomConstructor(name="no_crash")

    def test_child_class_no_custom_constructor(self):
        h = MockHashNoCustomConstructor(128)
        assert h.default_difficulty() == 64
        assert h.difficulty == h.default_difficulty()
        assert h.num_input_bits == 128
        assert h.hash_name() == "no_custom_constructor"

        rep = h.symbolic_representation()
        assert isinstance(rep, SymRepresentation)
        assert len(rep.input_indices) == h.num_input_bits
        assert len(rep.output_indices) == h.num_input_bits

        bv = SymBitVec(utils.random_bits(h.num_input_bits))
        output = h.forward(bv)  # Call `forward` with SymBitVec
        assert isinstance(output, SymBitVec)
        assert len(output) == 128

        output = h()  # Call on random input bytes
        assert isinstance(output, bytes)
        assert len(output) == 128 // 8

        input_data = utils.random_bits(h.num_input_bits)
        output = h(input_data)  # Call on given input bytes
        assert isinstance(output, bytes)
        assert len(output) == 128 // 8

    def test_bad_number_of_input_bits(self):
        with pytest.raises(ValueError):
            _ = MockHashNoCustomConstructor(31)

    def test_input_size_mismatch(self):
        h = MockHashNoCustomConstructor(32)
        with pytest.raises(ValueError):
            _ = h(utils.random_bits(24))

    def test_set_readonly_properties(self):
        h = MockHashNoCustomConstructor(128)
        with pytest.raises(AttributeError):
            h.num_input_bits = 256
        with pytest.raises(AttributeError):
            h.difficulty = 100
