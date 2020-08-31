# -*- coding: utf-8 -*-
import os
import sys
import math
import yaml
import argparse
import subprocess
import numpy as np
from time import time
from BitVector import BitVector
from scipy.optimize import minimize

from optimization.gnc import GNC
from optimization.factor import Factor


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
    n = int(config['num_rvs'])
    N = int(config['num_samples'])
    bv = BitVector(filename=data_file)
    data = bv.read_bits_from_file(n * N)
    bv.close_file_object()
    data = np.array([bit for bit in data], dtype=bool)
    data = data.reshape((n, N)).T  # data is (N x n)

    samples = []
    for sample_idx in range(N):
        sample = data[N - sample_idx - 1, :]
        samples.append(BitVector(bitlist=sample.astype(bool)))
    return samples


def solve(factors, observed, config):
    n = int(config['num_rvs'])
    init_guess = [0.5 for _ in range(n)]
    for rv, val in observed.items():
        init_guess[rv] = float(val)

    factors_per_rv = [set([i]) for i in range(n)]
    for i, factor in factors.items():
        for input_rv in factor.input_rvs:
            factors_per_rv[input_rv].add(i)

    # Ax = b (for SAME and INV) --> Ax - b = 0
    A = np.zeros((n, n))
    b = np.zeros((n, 1))

    for i in range(n):
        factor = factors[i]
        if factor.factor_type == 'SAME':
            inp, out = factor.input_rvs[0], factor.output_rv
            # x_i - x_j = 0.0 since the RVs are the same
            A[out, out] = 1.0
            A[out, inp] = -1.0
        elif factor.factor_type == 'INV':
            inp, out = factor.input_rvs[0], factor.output_rv
            # x_i + x_j = 1.0 since the RVs are inverses
            A[out, out] = 1.0
            A[out, inp] = 1.0
            b[out] = 1.0

    and_factor_indices = [i for i in range(n)
                          if factors[i].factor_type == 'AND']

    C = 0.02
    C_sq = C * C
    gnc = GNC(C)

    def cb(x):
        gnc.increment()

    def f(x):
        for i in and_factor_indices:
            factor = factors[i]
            inp1, inp2 = factor.input_rvs
            out = factor.output_rv
            # Update the A matrix s.t. inp1 * inp2 - out = 0.0
            A[out, out] = -1.0
            A[out, inp1] = x[inp2]

        r_sq = (A.dot(x.reshape((-1, 1))) - b) ** 2.0
        return np.sum(r_sq)
        # mu = gnc.mu(r_sq)
        # err = (r_sq * C_sq * mu) / (r_sq + mu * C_sq)
        # return np.sum(err)

    # Jacobian: J[i] = derivative of f(x) w.r.t. variable `i`
    def jacobian(x):
        J = np.zeros(n)
        for i in range(n):
            for factor_idx in factors_per_rv[i]:
                J[i] += factors[factor_idx].first_order(i, x)
        return J

    # Hessian: H[i, j] = derivative of J[j] w.r.t. variable `i`
    def hessian(x):
        H = np.zeros((n, n))
        for j in range(n):
            for factor_idx in factors_per_rv[j]:
                factor = factors[factor_idx]
                for i in factor.referenced_rvs:
                    H[i, j] += factor.second_order(j, i, x)
        assert np.sum(np.abs(H - H.T)) < 1e-5, 'Hessian is not symmetric'
        return H

    lower, upper = np.zeros(n), np.ones(n)
    for rv, val in observed.items():
        lower[rv] = float(val)
        upper[rv] = float(val)

    options = {'maxiter': 40, 'disp': True}

    start = time()
    print('Starting optimization...')
    result = minimize(f, init_guess, method='trust-ncg', bounds=Bounds(lower, upper),
                      jac=jacobian, hess=hessian, options=options, callback=cb)
    print('Optimization finished in %.2f s' % (time() - start))
    print('\tsuccess: {}'.format(result['success']))
    print('\tstatus:  {}'.format(result['message']))
    print('\terror:   {:.2f}'.format(result['fun']))

    return [(1.0 if rv > 0.5 else 0.0) for rv in result['x']]


def verify(true_input, predicted_input, config):
    input_rvs = config['input_rv_indices']
    hash_algo = config['hash']
    difficulty = config['difficulty']

    fmt = '{:0%dX}' % (len(true_input) // 4)
    true_in = fmt.format(int(true_input)).lower()
    pred_in = fmt.format(int(predicted_input)).lower()
    print('Hash input: %s' % true_in)
    print('Pred input: %s' % pred_in)

    cmd = ['python', '-m', 'dataset_generation.generate',
           '--num-input-bits', str(len(input_rvs)),
           '--hash-algo', hash_algo, '--difficulty', str(difficulty),
           '--hash-input']

    true_out = subprocess.run(cmd + [true_in],
                              stdout=subprocess.PIPE).stdout.decode('utf-8')
    pred_out = subprocess.run(cmd + [pred_in],
                              stdout=subprocess.PIPE).stdout.decode('utf-8')

    if true_out == pred_out:
        print('Hashes match: {}'.format(true_out))
    else:
        print('Expected:\n\t{}\nGot:\n\t{}'.format(true_out, pred_out))


def main(dataset):
    config_file = os.path.join(dataset, 'params.yaml')
    factor_file = os.path.join(dataset, 'factors.txt')
    data_file = os.path.join(dataset, 'data.bits')

    config = load_config(config_file)
    factors = load_factors(factor_file)
    bitvectors = load_bitvectors(data_file, config)

    n = int(config['num_rvs'])
    N = int(config['num_samples'])
    input_rvs = config['input_rv_indices']
    observed_rvs = set(config['hash_rv_indices'])
    assert n == len(factors), 'Expected %d factors, got %d' % (n, len(factors))
    print('%d random variables' % n)

    num_test = min(1, len(bitvectors))

    for test_case in range(num_test):
        print('Test case %d/%d' % (test_case + 1, num_test))
        sample = bitvectors[test_case]
        observed = {rv: bool(sample[rv]) for rv in observed_rvs}
        predictions = solve(factors, observed, config)
        predicted_input = BitVector(size=len(input_rvs))
        true_input = BitVector(size=len(input_rvs))
        for rv in input_rvs:
            predicted_input[rv] = bool(predictions[rv])
            true_input[rv] = sample[rv]

        verify(true_input, predicted_input, config)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Hash reversal via optimization')
    parser.add_argument(
        'dataset',
        type=str,
        help='Path to the dataset directory')
    args = parser.parse_args()
    main(args.dataset)
    print('Done.')
    sys.exit(0)
