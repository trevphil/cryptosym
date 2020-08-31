# -*- coding: utf-8 -*-
import os
import sys
import yaml
import argparse
import subprocess
import numpy as np
from BitVector import BitVector

from optimization.factor import Factor
from optimization.solver import solve


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
