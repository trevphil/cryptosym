import random
import numpy as np
from copy import deepcopy

from logic.problem import Problem
from logic.gate import GateType, LogicGate, inputs_for_gate


class ProblemGenerator(object):
    def __init__(self):
        self.gate_types = [t for t in GateType]
        self.gate_weights = [0.077, 0.326, 0.024, 0.154, 0.419]

    def rand_flip(self, x):
        return x * (1 if random.random() < 0.5 else -1)

    def sample_gate(self, output, leaves, pool, node_depth):
        gate_type = np.random.choice(self.gate_types, p=self.gate_weights)
        num_inputs = inputs_for_gate(gate_type)
        inputs = []
        while len(inputs) == 0 or len(inputs) != len(set(inputs)):
            inputs = random.sample(leaves, k=1)
            inputs += list(random.sample(pool, k=num_inputs - 1))
        depth = max(node_depth[abs(inp)] for inp in inputs) + 1
        return LogicGate(gate_type, output=output,
                         inputs=inputs, depth=depth)

    def create_problem(self, input_size, output_size, num_gates):
        pool = []
        leaves = set()
        gates = []
        node_depth = dict()

        # Assign random values to inputs of the hash function
        for var in range(1, input_size + 1):
            literal = self.rand_flip(var)
            pool.append(literal)
            leaves.add(literal)
            node_depth[abs(literal)] = 0

        max_var = input_size + 1

        for _ in range(num_gates):
            gate = self.sample_gate(max_var, leaves, pool, node_depth)

            for inp in gate.inputs:
                if inp in leaves:
                    leaves.remove(inp)

            gates.append(gate)
            node_depth[gate.output] = gate.depth
            literal = self.rand_flip(gate.output)
            pool.append(literal)
            leaves.add(literal)

            max_var += 1

        input_indices = list(range(1, input_size + 1))
        if output_size >= len(pool):
            output_indices = pool
        else:
            output_indices = pool[-output_size:]
        
        """
        input_size = 3
        output_size = 1
        num_gates = 1
        gates = [
            LogicGate(GateType.xor_gate, inputs=[1, 2], output=3, depth=1)
        ]
        input_indices = [1, 2]
        output_indices = [3]
        max_var = gates[0].output + 1
        """

        problem = Problem(input_size=len(input_indices),
                          output_size=len(output_indices),
                          num_vars=(max_var - 1),
                          num_gates=len(gates),
                          input_indices=input_indices,
                          output_indices=output_indices,
                          gates=gates)
        return problem
