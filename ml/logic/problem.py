import os
import torch
import random
from collections import defaultdict

from logic.gate import LogicGate, GateType


class Problem(object):
    def __init__(self, input_size, output_size, num_vars, num_gates,
                 input_indices, output_indices, gates):
        self.input_size = input_size
        self.output_size = output_size
        self.num_vars = num_vars
        self.num_gates = num_gates

        self.input_indices = input_indices
        self.output_indices = output_indices
        self.gates = list(sorted(gates, key=lambda gate: gate.output))

        assert isinstance(self.input_size, int), 'Not all properties initialized'
        assert isinstance(self.output_size, int), 'Not all properties initialized'
        assert isinstance(self.num_vars, int), 'Not all properties initialized'
        assert isinstance(self.num_gates, int), 'Not all properties initialized'
        assert isinstance(self.input_indices, list), 'Not all properties initialized'
        assert isinstance(self.output_indices, list), 'Not all properties initialized'

        k = len(self.input_indices)
        if self.input_size != k:
            assert False, f'Expected {self.input_size} input indices but got {k}'
        k = len(self.output_indices)
        if self.output_size != k:
            assert False, f'Expected {self.output_size} output indices but got {k}'
        k = len(self.gates)
        if self.num_gates != k:
            assert False, f'Expected {self.num_gates} logic gates but got {k}'

        self.bit_depths = defaultdict(lambda: 0)
        for gate in self.gates:
            self.bit_depths[gate.output] = gate.depth
        self.max_depth = max(self.bit_depths.values())

    @staticmethod
    def from_file(symbols_filename):
        tup = Problem.parse_symbols(symbols_filename)
        in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates = tup

        return Problem(input_size=in_sz, output_size=out_sz,
                       num_vars=n_vars, num_gates=n_gates,
                       input_indices=in_idx, output_indices=out_idx,
                       gates=gates)

    @staticmethod
    def parse_symbols(filename):
        if not os.path.exists(filename) or not os.path.isfile(filename):
            assert False, 'File not found! File name: %s' % filename

        in_sz, out_sz, n_vars, n_gates = None, None, None, None
        in_idx, out_idx = None, None
        gates = []

        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if len(line) == 0 or line.startswith('#'):
                    continue  # skip empty lines and comments

                if in_sz is None:
                    parts = [int(x) for x in line.split(' ')]
                    in_sz, out_sz, n_vars, n_gates = parts
                elif in_idx is None:
                    in_idx = [int(x) for x in line.split(' ')]
                elif out_idx is None:
                    out_idx = [int(x) for x in line.split(' ')]
                else:
                    parts = line.split(' ')
                    gate_type = GateType(parts[0])
                    depth = int(parts[1])
                    out = int(parts[2])
                    inputs = [int(x) for x in parts[3:]]
                    gate = LogicGate(gate_type, output=out,
                                     inputs=inputs, depth=depth)
                    gates.append(gate)

        return (in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates)

    def forward_computation(self, bits):
        # Propagate through directed graph, assume topological sort
        assert isinstance(bits, torch.Tensor)

        for gate in self.gates:
            inputs = []
            for inp in gate.inputs:
                inp_val = bits[abs(inp) - 1]
                if inp < 0:
                    inp_val = 1 - inp_val
                inputs.append(inp_val)
            bits[gate.output - 1] = gate.compute_output(inputs)

        return bits

    def random_bits(self, rng=None):
        bits = torch.ones(self.num_vars, dtype=torch.int32) * -1

        if rng is None:
            rand_bits = [(random.random() > 0.5) for _ in range(self.input_size)]
        else:
            rand_bits = rng.integers(low=0, high=1, size=self.input_size,
                                     dtype=int, endpoint=True)

        # Random input bits
        for idx, inp in enumerate(self.input_indices):
            if inp != 0:
                bits[abs(inp) - 1] = int(rand_bits[idx])

        # Forward computation
        for gate in self.gates:
            inputs = []
            for inp in gate.inputs:
                inp_val = bits[abs(inp) - 1]
                assert inp_val != -1, 'Input bit is UNSET'
                if inp < 0:
                    inp_val = 1 - inp_val
                inputs.append(inp_val)
            bits[gate.output - 1] = gate.compute_output(inputs)

        # Get output bits
        observed = dict()
        for out in self.output_indices:
            if out != 0:
                observed[abs(out)] = bits[abs(out) - 1].item()

        assert not (bits == -1).any(), 'Some bits are unset!'
        return bits.type(torch.uint8), observed
