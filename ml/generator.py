import random
import numpy as np
from copy import deepcopy

from problem import Problem
from logic_gate import GateType, LogicGate, inputs_for_gate


class ProblemGenerator(object):
    def __init__(self, opts):
        self.opts = opts

    def rand_flip(self, x):
        return x * (1 if random.random() < 0.5 else -1)

    def sample_gate(self, output, leaves, pool):
        weights = [0.077, 0.326, 0.024, 0.154, 0.419]
        types = [t for t in GateType]
        gate_type = np.random.choice(types, p=weights)
        num_inputs = inputs_for_gate(gate_type)
        inputs = []
        while len(inputs) == 0 or len(inputs) != len(set(inputs)):
            inputs = random.sample(leaves, k=1)
            inputs += list(random.sample(pool, k=num_inputs - 1))
        gate = LogicGate(gate_type, output=output, inputs=inputs)
        return gate

    def create_problem(self, input_size, output_size, num_gates):
        pool = []
        leaves = set()
        gates = []

        # Assign random values to inputs of the hash function
        for var in range(1, input_size + 1):
            literal = self.rand_flip(var)
            pool.append(literal)
            leaves.add(literal)

        max_var = input_size + 1

        for _ in range(num_gates):
            gate = self.sample_gate(max_var, leaves, pool)

            for inp in gate.inputs:
                if inp in leaves:
                    leaves.remove(inp)

            gates.append(gate)
            literal = self.rand_flip(gate.output)
            pool.append(literal)
            leaves.add(literal)

            max_var += 1

        input_indices = list(range(1, input_size + 1))
        if output_size >= len(pool):
            output_indices = pool
        else:
            output_indices = pool[-output_size:]

        num_vars = (max_var - 1)
        adj_mat_data = []

        def _add_clause(lits, ci):
            for lit in lits:
                row = abs(lit) - 1
                val = 1 if lit > 0 else -1
                adj_mat_data.append((row, ci, val))
            return ci + 1

        clause_idx = 0
        for gate in gates:
            clauses = gate.cnf_clauses()
            for clause in clauses:
                clause_idx = _add_clause(clause, clause_idx)
        num_clauses = clause_idx

        problem = Problem(input_size=len(input_indices),
                          output_size=len(output_indices),
                          num_vars=num_vars,
                          num_gates=len(gates),
                          num_clauses=num_clauses,
                          input_indices=input_indices,
                          output_indices=output_indices,
                          adj_mat_data=adj_mat_data,
                          gates=gates)
        return problem


if __name__ == '__main__':
    from time import time

    from opts import PreimageOpts
    from visualization import plot_dag, plot_cnf

    opts = PreimageOpts()
    gen = ProblemGenerator(opts)

    input_size = 16
    output_size = 16
    num_gates = 128

    start = time()
    problem = gen.create_problem(input_size, output_size, num_gates)
    runtime_ms = (time() - start) * 1000.0
    print(f'Generated problem in {runtime_ms} ms')
    print(f'\tin={input_size}, out={output_size}, gates={num_gates}')

    # plot_dag(problem)
    plot_cnf(problem)
