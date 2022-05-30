from math import ceil

import pytest


class Helpers:
    @staticmethod
    def int_to_little_endian_bytes(n: int, num_bits: int) -> bytes:
        num_bytes = int(ceil(num_bits / 8))
        return n.to_bytes(length=num_bytes, byteorder="little")

    @staticmethod
    def reverse_endianness(data: bytes) -> bytes:
        data = bytearray(data)
        data.reverse()
        return bytes(data)

    @staticmethod
    def build_assignments(
        hash_output: bytes, hash_output_indices: list[int]
    ) -> dict[int, bool]:
        hash_output = int.from_bytes(hash_output, byteorder="little")
        hash_output = bin(hash_output)[2:][::-1]
        bit_assignments = {}
        for i, var in zip(hash_output_indices, hash_output):
            var = {"0": False, "1": True}[var]
            if i < 0:
                bit_assignments[-i] = not var
            elif i > 0:
                bit_assignments[i] = var
        return bit_assignments

    @staticmethod
    def build_input_bytes(
        bit_assignments: dict[int, bool], hash_input_indices: list[int]
    ) -> bytes:
        hash_input_bits = ""
        for i in hash_input_indices:
            if i < 0 and -i in bit_assignments:
                hash_input_bits += str(int(not bit_assignments[-i]))
            elif i > 0 and i in bit_assignments:
                hash_input_bits += str(int(bit_assignments[i]))
            else:
                hash_input_bits += "0"

        n_bytes = len(hash_input_bits) // 8
        hash_input_bits = hash_input_bits[::-1]
        return int(hash_input_bits, 2).to_bytes(n_bytes, byteorder="little")


@pytest.fixture
def helpers() -> Helpers:
    return Helpers()
