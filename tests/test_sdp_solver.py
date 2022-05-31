"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

import numpy as np
import pytest

import cryptosym
from cryptosym import SDPSolver

HASH_AND_DIFFICULTY = (
    (cryptosym.SymMD5, 12),
    (cryptosym.SymRIPEMD160, 12),
    (cryptosym.SymSHA256, 16),
    (cryptosym.SymSHA512, 8),
)


class TestSDPSolver:
    @pytest.mark.parametrize("data", HASH_AND_DIFFICULTY)
    def test_preimage(self, data: tuple[cryptosym.SymHash, int], helpers):
        cryptosym.utils.seed(37)

        hash_class, difficulty = data
        hasher = hash_class(64, difficulty=difficulty)
        hash_output = hasher()
        hash_hex = cryptosym.utils.hexstr(raw_bytes=hash_output)
        problem = hasher.symbolic_representation()
        solver = SDPSolver()
        assert solver.solver_name() == "SDP Mixing Method"

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
        solver = SDPSolver()
        with pytest.raises(ValueError):
            _ = solver.solve(problem, bit_assignments=assignments)

    def test_unsatisfiable_problem(self):
        g = cryptosym.LogicGate("A 3 1 -2")
        problem = cryptosym.SymRepresentation([g], [1, -2], [3])
        assignments = {3: True, 1: True, 2: True}
        solver = SDPSolver()
        with pytest.raises(RuntimeError):
            _ = solver.solve(problem, bit_assignments=assignments)

    def test_approximate_solve_md5(self):
        solver = SDPSolver(num_rounding_trials=100)
        md5 = cryptosym.SymMD5(64, 20)
        problem = md5.symbolic_representation()
        cnf = problem.to_cnf()

        for _ in range(4):
            hash_out = md5()
            solution = solver.solve(problem, hash_output=hash_out)
            approx_ratio = cnf.approximation_ratio(solution)
            assert approx_ratio >= 0.92

    def test_sdp_embedding(self, helpers):
        sha256 = cryptosym.SymSHA256(64, difficulty=17)
        problem = sha256.symbolic_representation()
        bit_dict = helpers.build_assignments(sha256(), problem.output_indices)
        cnf, _, _ = problem.to_cnf().simplify(bit_dict)

        solver = SDPSolver(num_rounding_trials=10)
        v1 = solver.mixing_method(cnf)
        v2 = solver.mixing_method(cnf=cnf)
        assert isinstance(v1, np.ndarray)
        assert isinstance(v2, np.ndarray)
        assert v1.shape == v2.shape
        assert v1.ndim == 2
        assert v1.shape[0] == cnf.num_vars + 1
        assert not np.allclose(v1, v2)

    def test_mixing_method_with_empty_cnf(self):
        cnf = cryptosym.CNF(clauses=[], num_vars=0)
        solver = SDPSolver()
        with pytest.raises(RuntimeError):
            _ = solver.mixing_method(cnf)
