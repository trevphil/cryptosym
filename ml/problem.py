import os
import torch
import random

from logic_gate import LogicGate, GateType


class Problem(object):
    def __init__(self, input_size, output_size, num_vars, num_gates, num_clauses,
                 input_indices, output_indices, adj_mat_data, gates):
        self.input_size = input_size
        self.output_size = output_size
        self.num_vars = num_vars
        self.num_gates = num_gates
        self.num_clauses = num_clauses

        self.input_indices = input_indices
        self.output_indices = output_indices
        self.adj_mat_data = adj_mat_data
        self.gates = list(sorted(gates, key=lambda gate: gate.output))

        assert isinstance(self.input_size, int), 'Not all properties initialized'
        assert isinstance(self.output_size, int), 'Not all properties initialized'
        assert isinstance(self.num_vars, int), 'Not all properties initialized'
        assert isinstance(self.num_gates, int), 'Not all properties initialized'
        assert isinstance(self.num_clauses, int), 'Not all properties initialized'
        assert isinstance(self.input_indices, list), 'Not all properties initialized'
        assert isinstance(self.output_indices, list), 'Not all properties initialized'
        assert isinstance(self.adj_mat_data, list), 'Not all properties initialized'
        k = len(self.input_indices)
        if self.input_size != k:
            assert False, 'Expected %d input indices but got %d' % (self.input_size, k)
        k = len(self.output_indices)
        if self.output_size != k:
            assert False, 'Expected %d output indices but got %d' % (self.output_size, k)
        k = len(self.gates)
        if self.num_gates != k:
            assert False, 'Expected %d logic gates but got %d' % (self.m, k)

        self.A = self.bipartite_adjacency_mat(transpose=False)
        self.A_T = self.bipartite_adjacency_mat(transpose=True)

    @staticmethod
    def from_files(symbols_filename, cnf_filename):
        tup = Problem.parse_symbols(symbols_filename)
        in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates = tup

        tup = Problem.parse_dimacs(cnf_filename, n_vars)
        n_clauses, adj_mat_data = tup

        return Problem(input_size=in_sz, output_size=out_sz, num_vars=n_vars,
                       num_gates=n_gates, num_clauses=n_clauses,
                       input_indices=in_idx, output_indices=out_idx,
                       adj_mat_data=adj_mat_data, gates=gates)

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
                    out = int(parts[2])
                    inputs = [int(x) for x in parts[3:]]
                    gate = LogicGate(gate_type, output=out, inputs=inputs)
                    gates.append(gate)

        return (in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates)

    @staticmethod
    def parse_dimacs(filename, num_vars, skip_assignments=True):
        if not os.path.exists(filename) or not os.path.isfile(filename):
            assert False, 'File not found! File name: %s' % filename

        line_no = 0
        clause_idx = 0
        adj_mat_data = []

        with open(filename, 'r') as f:
            for line in f:
                line_no += 1
                line = line.strip()
                if len(line) == 0 or line.startswith('#') or line.startswith('p cnf'):
                    continue  # skip empty lines, comments, and header

                lits = [int(x) for x in line.split(' ')][:-1]  # remove final 0
                if skip_assignments and len(lits) == 1:
                    continue  # skip clauses which assign values to single literals

                for x in lits:
                    xa = abs(x)
                    assert 0 < xa <= num_vars, 'Invalid CNF, line %d: %s' % (
                        line_no, line)
                    if x < 0:
                        adj_mat_data.append((xa - 1, clause_idx, -1))  # negated
                    else:
                        adj_mat_data.append((xa - 1, clause_idx, 1))  # (row, col)
                clause_idx += 1

        num_clauses = clause_idx
        return (num_clauses, adj_mat_data)

    def forward_computation(self, assignments):
        # Propagate through directed graph, assume topological sort
        for gate in self.gates:
            inputs = []
            for inp in gate.inputs:
                inp_val = assignments[abs(inp)]
                if inp < 0:
                    inp_val = 1 - inp_val
                inputs.append(inp_val)
            assignments[gate.output] = gate.compute_output(inputs)
        return assignments

    def random_observed(self, rng=None):
        assignments = dict()
        
        if rng is None:
            input_bits = [int(random.random() > 0.5) for _ in range(self.input_size)]
        else:
            input_bits = rng.integers(low=0, high=1, size=self.input_size,
                                      dtype=int, endpoint=True)

        # Random input bits
        for idx, inp in enumerate(self.input_indices):
            if inp != 0:
                assignments[abs(inp)] = input_bits[idx]

        assignments = self.forward_computation(assignments)

        # Get output bits
        observed = dict()
        for out in self.output_indices:
            if out > 0:
                observed[out] = assignments[out]
            elif out < 0:
                observed[-out] = assignments[-out]
        return observed

    def bipartite_adjacency_mat(self, transpose=False):
        if transpose:
            indices = [(y, x) for (x, y, v) in self.adj_mat_data]
            values = [v for (x, y, v) in self.adj_mat_data]
            sz = (self.num_clauses, self.num_vars)
        else:
            indices = [(x, y) for (x, y, v) in self.adj_mat_data]
            values = [v for (x, y, v) in self.adj_mat_data]
            sz = (self.num_vars, self.num_clauses)

        return torch.sparse_coo_tensor(
            list(zip(*indices)), values,
            size=sz, dtype=torch.float32)


if __name__ == '__main__':
    from time import time
    sym_filename = os.path.join('samples', 'sha256_d8_sym.txt')
    cnf_filename = os.path.join('samples', 'sha256_d8_cnf.txt')
    print('Loading problem: (%s, %s)' % (sym_filename, cnf_filename))

    start = time()
    problem = Problem.from_files(sym_filename, cnf_filename)
    runtime = time() - start
    print('Loaded problem in %.2f ms' % (1000.0 * runtime))

    start = time()
    observed = problem.random_observed()
    runtime = time() - start
    print('Computed random observed bits in %.2f ms' % (1000.0 * runtime))

    print('Done.')
