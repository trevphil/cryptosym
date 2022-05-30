import pytest

import cryptosym
from cryptosym import CMSatSolver

HASH_AND_DIFFICULTY = (
    (cryptosym.SymMD5, 12),
    (cryptosym.SymRIPEMD160, 12),
    (cryptosym.SymSHA256, 8),
    (cryptosym.SymSHA512, 8),
)


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


class TestCMSatSolver:
    @pytest.mark.parametrize("data", HASH_AND_DIFFICULTY)
    def test_preimage(self, data: tuple[cryptosym.SymHash, int]):
        cryptosym.utils.seed(42)

        hash_class, difficulty = data
        hasher = hash_class(64, difficulty=difficulty)
        hash_output = hasher()
        hash_hex = cryptosym.utils.hexstr(raw_bytes=hash_output)
        problem = hasher.symbolic_representation()
        solver = CMSatSolver()

        solution = solver.solve(problem, hash_output=hash_output)
        preimage = build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

        solution = solver.solve(problem, hash_hex=hash_hex)
        preimage = build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

        assignments = build_assignments(hash_output, problem.output_indices)
        solution = solver.solve(problem, bit_assignments=assignments)
        preimage = build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

    def test_negated_bit_assignments(self):
        g = cryptosym.LogicGate("A 3 1 -2")
        problem = cryptosym.SymRepresentation([g], [1, -2], [3])
        assignments = {3: True, -2: True}
        solver = CMSatSolver()
        with pytest.raises(ValueError):
            _ = solver.solve(problem, bit_assignments=assignments)

    def test_unsatisfiable_problem(self):
        g = cryptosym.LogicGate("A 3 1 -2")
        problem = cryptosym.SymRepresentation([g], [1, -2], [3])
        assignments = {3: True, 1: True, 2: True}
        solver = CMSatSolver()
        with pytest.raises(RuntimeError):
            _ = solver.solve(problem, bit_assignments=assignments)
