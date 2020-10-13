# -*- coding: utf-8 -*-

import os
import sys
import yaml
import argparse
import subprocess
import numpy as np
from time import time
from BitVector import BitVector

from optimization import utils
from optimization.factor import Factor
from optimization.gradient_solver import GradientSolver
from optimization.gnc_solver import GNCSolver
from optimization.cplex_milp_solver import CplexMILPSolver
from optimization.ortools_cp_solver import OrtoolsCpSolver
from optimization.cplex_cp_solver import CplexCPSolver
from optimization.gurobi_milp_solver import GurobiMILPSolver
from optimization.sympy_solver import SympySolver

def load_factors(factor_file):
    factors = dict()
    with open(factor_file, 'r') as f:
        for line in f:
            factor = Factor(line)
            factors[factor.output_rv] = factor
    return factors


def load_config(config_file):
    with open(config_file, 'r') as f:
        config = yaml.safe_load(f)
    return config


def load_bitvectors(data_file, config):
    n = int(config['num_bits_per_sample'])
    N = int(config['num_samples'])
    bv = BitVector(filename=data_file)
    data = bv.read_bits_from_file(n * N)
    bv.close_file_object()
    data = np.array([bit for bit in data], dtype=bool)
    data = data.reshape((N, n))

    samples = []
    for sample_idx in range(N):
        sample = data[sample_idx, :]
        samples.append(BitVector(bitlist=sample.astype(bool)))
    return samples


def select_solver(solver_type):
    if solver_type == 'cplex_milp':
        return CplexMILPSolver()
    elif solver_type == 'cplex_cp':
        return CplexCPSolver()
    elif solver_type == 'gradient':
        return GradientSolver()
    elif solver_type == 'gnc':
        return GNCSolver()
    elif solver_type == 'ortools_cp':
        return OrtoolsCpSolver()
    elif solver_type == 'gurobi_milp':
        return GurobiMILPSolver()
    elif solver_type == 'sympy':
        return SympySolver()
    else:
        raise NotImplementedError('Invalid solver: %s' % solver_type)


def verify(true_input, predicted_input, config):
    num_input_bits = config['num_input_bits']
    hash_algo = config['hash']
    difficulty = config['difficulty']

    fmt = '{:0%dX}' % (len(true_input) // 4)
    true_in = fmt.format(int(true_input)).lower()
    pred_in = fmt.format(int(predicted_input)).lower()
    print('Hash input: %s' % true_in)
    print('Pred input: %s' % pred_in)

    cmd = ['python', '-m', 'dataset_generation.generate',
           '--num-input-bits', str(num_input_bits),
           '--hash-algo', hash_algo, '--difficulty', str(difficulty),
           '--hash-input']

    true_out = subprocess.run(cmd + [true_in],
                              stdout=subprocess.PIPE).stdout.decode('utf-8')
    pred_out = subprocess.run(cmd + [pred_in],
                              stdout=subprocess.PIPE).stdout.decode('utf-8')

    if true_out == pred_out:
        print('Hashes match: {}'.format(true_out))
        return True
    else:
        print('Expected:\n\t{}\nGot:\n\t{}'.format(true_out, pred_out))
        return False


def main(dataset, solver_type):
    config_file = os.path.join(dataset, 'params.yaml')
    factor_file = os.path.join(dataset, 'factors.txt')
    data_file = os.path.join(dataset, 'data.bits')

    config = load_config(config_file)
    factors = load_factors(factor_file)
    bitvectors = load_bitvectors(data_file, config)

    n = int(config['num_bits_per_sample'])
    n_input = int(config['num_input_bits'])
    N = int(config['num_samples'])
    observed_rvs = set(config['observed_rv_indices'])
    num_test = min(1, len(bitvectors))

    solver = select_solver(solver_type)

    stats = {
        'problem_size': [],
        'runtime': [],
        'difficulty': [],
        'success': []
    }

    for test_case in range(num_test):
        print('Test case %d/%d' % (test_case + 1, num_test))
        sample = bitvectors[test_case]
        observed = {rv: bool(sample[rv]) for rv in observed_rvs}
        observed = utils.set_implicit_observed(factors, observed, sample)

        start = time()
        if len(observed) == len(factors):
            predictions = observed  # Everything was solved already :)
        else:
            predictions = solver.solve(factors, observed, config, sample)

        stats['runtime'].append(time() - start)
        stats['problem_size'].append(len(factors))
        stats['difficulty'].append(config['difficulty'])

        predicted_input = BitVector(size=n_input)
        true_input = sample[:n_input]
        for rv_idx, predicted_val in predictions.items():
            if rv_idx < n_input:
                predicted_input[rv_idx] = bool(predicted_val)

        success = verify(true_input, predicted_input, config)
        stats['success'].append(float(success))
    return stats


if __name__ == '__main__':
    np.random.seed(1)
    parser = argparse.ArgumentParser(
        description='Hash reversal via optimization')
    parser.add_argument('dataset', type=str,
        help='Path to the dataset directory')
    choices = ['gradient', 'gnc', 'cplex_milp', 'cplex_cp',
        'ortools_cp', 'gurobi_milp', 'sympy']
    parser.add_argument('--solver', type=str, default='sat',
        help='The solving technique', choices=choices)
    args = parser.parse_args()
    _ = main(args.dataset, args.solver)
    print('Done.')
    sys.exit(0)
