import os
import torch
import random


class AndGate(object):
    def __init__(self, out, inp1, inp2):
        assert (type(out) is int) and (type(inp1) is int) and (type(inp2) is int)
        assert 0 not in (out, inp1, inp2), 'Bad AND gate: (%d, %d, %d)' % (
            out, inp1, inp2)
        assert out > 0, 'AND output should be positive! output = %d' % out
        self.out = out
        self.inp1 = inp1
        self.inp2 = inp2

    def __hash__(self):
        return hash(f'{self.out} {self.inp1} {self.inp2}')


class Problem(object):
    def __init__(self, input_size, output_size, num_vars, num_gates, num_clauses,
                 input_indices, output_indices, cnf_indices, and_gates):
        self.input_size = input_size
        self.output_size = output_size
        self.num_vars = num_vars
        self.num_gates = num_gates
        self.num_clauses = num_clauses

        self.input_indices = input_indices
        self.output_indices = output_indices
        self.cnf_indices = cnf_indices
        self.and_gates = list(sorted(and_gates, key=lambda gate: gate.out))
        self.observed = dict()

        assert isinstance(self.input_size, int), 'Not all properties initialized'
        assert isinstance(self.output_size, int), 'Not all properties initialized'
        assert isinstance(self.num_vars, int), 'Not all properties initialized'
        assert isinstance(self.num_gates, int), 'Not all properties initialized'
        assert isinstance(self.num_clauses, int), 'Not all properties initialized'
        assert isinstance(self.input_indices, list), 'Not all properties initialized'
        assert isinstance(self.output_indices, list), 'Not all properties initialized'
        assert isinstance(self.cnf_indices, list), 'Not all properties initialized'
        k = len(self.input_indices)
        if self.input_size != k:
            assert False, 'Expected %d input indices but got %d' % (self.input_size, k)
        k = len(self.output_indices)
        if self.output_size != k:
            assert False, 'Expected %d output indices but got %d' % (self.output_size, k)
        k = len(self.and_gates)
        if self.num_gates != k:
            assert False, 'Expected %d logic gates but got %d' % (self.m, k)

        self.A = self.bipartite_adjacency_mat(transpose=False)
        self.A_T = self.bipartite_adjacency_mat(transpose=True)

    @staticmethod
    def from_files(symbols_filename, cnf_filename):
        tup = Problem.parse_symbols(symbols_filename)
        in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates = tup

        tup = Problem.parse_dimacs(cnf_filename, n_vars)
        n_clauses, cnf_indices = tup

        return Problem(input_size=in_sz, output_size=out_sz, num_vars=n_vars,
                       num_gates=n_gates, num_clauses=n_clauses,
                       input_indices=in_idx, output_indices=out_idx,
                       cnf_indices=cnf_indices, and_gates=gates)

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
                    if parts[0] != 'A':
                        assert False, 'Only AND gates are supported!'
                    out = int(parts[2])
                    inp1 = int(parts[3])
                    inp2 = int(parts[4])
                    gate = AndGate(out, inp1, inp2)
                    gates.append(gate)

        return (in_sz, out_sz, n_vars, n_gates, in_idx, out_idx, gates)

    @staticmethod
    def parse_dimacs(filename, num_vars, skip_assignments=True):
        if not os.path.exists(filename) or not os.path.isfile(filename):
            assert False, 'File not found! File name: %s' % filename

        line_no = 0
        clause_idx = 0
        cnf_indices = []

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
                        cnf_indices.append((xa - 1 + num_vars, clause_idx))  # negated
                    else:
                        cnf_indices.append((xa - 1, clause_idx))  # (row, col)
                clause_idx += 1

        num_clauses = clause_idx
        return (num_clauses, cnf_indices)

    def set_observed(self, obs):
        for var, assignment in obs.items():
            assert 0 < var <= self.num_vars, 'Invalid variable index for assignment %d' % var
        self.observed = obs

    def randomize_observed(self):
        self.observed = dict()
        assignments = dict()

        # Random input bits
        for inp in self.input_indices:
            if inp != 0:
                assignments[abs(inp)] = int(random.random() > 0.5)

        # Propagate through directed graph, assume topological sort
        for gate in self.and_gates:
            inp1_idx = abs(gate.inp1)
            inp2_idx = abs(gate.inp2)
            assert inp1_idx in assignments
            assert inp2_idx in assignments
            inp1 = assignments[inp1_idx]
            if gate.inp1 < 0:
                inp1 = 1 - inp1
            inp2 = assignments[inp2_idx]
            if gate.inp2 < 0:
                inp2 = 1 - inp2
            assignments[gate.out] = (inp1 * inp2)

        # Get output bits
        for out in self.output_indices:
            if out != 0:
                self.observed[abs(out)] = assignments[abs(out)]

    def bipartite_adjacency_mat(self, transpose=False):
        if transpose:
            indices = [(y, x) for (x, y) in self.cnf_indices]
            sz = (self.num_clauses, int(2 * self.num_vars))
        else:
            indices = self.cnf_indices
            sz = (int(2 * self.num_vars), self.num_clauses)

        return torch.sparse_coo_tensor(
            list(zip(*indices)),
            torch.ones(len(indices), dtype=torch.float32),
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

    obs = {1: True, 2: False, 3: True, 10: False}
    problem.set_observed(obs)

    print('Done.')
