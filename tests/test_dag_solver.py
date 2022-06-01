"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

import pytest

import cryptosym
from cryptosym import DAGSolver

HASH_AND_DIFFICULTY = (
    (cryptosym.SymMD5, 4),
    (cryptosym.SymRIPEMD160, 4),
    (cryptosym.SymSHA256, 4),
    (cryptosym.SymSHA512, 4),
)


class TestDAGSolver:
    @pytest.mark.parametrize("data", HASH_AND_DIFFICULTY)
    def test_preimage(self, data: tuple[cryptosym.SymHash, int], helpers):
        cryptosym.utils.seed(14)

        hash_class, difficulty = data
        hasher = hash_class(64, difficulty=difficulty)
        hash_output = hasher()
        hash_hex = cryptosym.utils.hexstr(raw_bytes=hash_output)
        problem = hasher.symbolic_representation()
        solver = DAGSolver()
        assert solver.solver_name() == "DAG Solver"

        solution = solver.solve(problem, hash_output=hash_output)
        preimage = helpers.build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

        solution = solver.solve(problem, hash_hex=hash_hex)
        preimage = helpers.build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

        assignments = helpers.build_assignments(hash_output, problem.output_indices)
        solution = solver.solve(problem, bit_assignments=assignments)
        preimage = helpers.build_input_bytes(solution, problem.input_indices)
        assert hash_output == hasher(preimage)

    def test_negated_bit_assignments(self):
        g = cryptosym.LogicGate("A 3 1 -2")
        problem = cryptosym.SymRepresentation([g], [1, -2], [3])
        assignments = {3: True, -2: True}
        solver = DAGSolver()
        with pytest.raises(ValueError):
            _ = solver.solve(problem, bit_assignments=assignments)

    def test_unsatisfiable_problem(self):
        g = cryptosym.LogicGate("A 3 1 -2")
        problem = cryptosym.SymRepresentation([g], [1, -2], [3])
        assignments = {3: True, 1: True, 2: True}
        solver = DAGSolver()
        with pytest.raises(RuntimeError):
            _ = solver.solve(problem, bit_assignments=assignments)
