"""
Copyright (c) 2022 Authors:
    - Trevor Phillips <trevphil3@gmail.com>

Distributed under the CC BY-NC-SA 4.0 license
(See accompanying file LICENSE.md).
"""

from multiprocessing import cpu_count

from ortools.linear_solver import pywraplp

from ._cpp import LogicGate, Solver, SymRepresentation


class ORToolsMIPSolver(Solver):
    def __init__(self, num_threads: int = cpu_count()):
        Solver.__init__(self)
        self._num_threads = num_threads

    def solver_name(self) -> str:
        return "OR-Tools Mixed Integer Programming"

    @staticmethod
    def _get_var(
        i: int, cache: dict[int, pywraplp.Variable], solver: pywraplp.Solver
    ) -> pywraplp.Variable:
        j = abs(i)
        if j not in cache:
            cache[j] = solver.BoolVar(f"x{j}")
        return cache[j] if i == j else 1 - cache[j]

    @staticmethod
    def _int_to_dict(problem: SymRepresentation, data: int) -> dict[int, bool]:
        little_endian_bin_str = bin(data)[2:][::-1]
        bit_assignments = {}
        for i, var in zip(problem.output_indices, little_endian_bin_str):
            var = {"0": False, "1": True}[var]
            if i < 0:
                bit_assignments[-i] = not var
            elif i > 0:
                bit_assignments[i] = var
        return bit_assignments

    def solve(
        self,
        problem: SymRepresentation,
        hash_output: bytes = None,
        hash_hex: str = None,
        bit_assignments: dict[int, bool] = None,
    ) -> dict[int, bool]:
        if (
            int(hash_output is not None)
            + int(hash_hex is not None)
            + int(bit_assignments is not None)
            != 1
        ):
            raise ValueError(
                "Exactly one of `hash_output`, `hash_hex`, `bit_assignments` "
                "should be provided as an argument"
            )

        if hash_output is not None:
            data = int.from_bytes(hash_output, byteorder="little")
            bit_assignments = self._int_to_dict(problem, data)
        elif hash_hex is not None:
            data = int(hash_hex, 16)
            bit_assignments = self._int_to_dict(problem, data)

        ptype = pywraplp.Solver.SAT_INTEGER_PROGRAMMING
        solver = pywraplp.Solver("CBC", ptype)
        solver.SetNumThreads(self._num_threads)

        cache = {}

        for i, value in bit_assignments.items():
            var = self._get_var(i, cache, solver)
            solver.Add(var == bool(value))

        for gate in problem.gates:
            if gate.t != LogicGate.Type.and_gate:
                raise ValueError(
                    "ORToolsMIPSolver only works with AND gates. "
                    "Please set `cryptosym.config.only_and_gates = True` "
                    "and try again."
                )

            inp1 = self._get_var(gate.inputs[0], cache, solver)
            inp2 = self._get_var(gate.inputs[1], cache, solver)
            out = self._get_var(gate.output, cache, solver)

            # Linearization of out = inp1 * inp2 for binary variables
            solver.Add(out <= inp1)
            solver.Add(out <= inp2)
            solver.Add(out >= inp1 + inp2 - 1)

        solver.Maximize(sum(cache.values()))
        result = solver.Solve()
        if result != 0:
            raise RuntimeError("OR-Tools could not solve the problem!")
        return {i: var.solution_value() for i, var in cache.items()}
