# -*- coding: utf-8 -*-
import os
import sys
import math
import yaml
import subprocess
import numpy as np
from time import time
from BitVector import BitVector

from scipy.optimize import (
  minimize, NonlinearConstraint, Bounds
)


class Factor(object):
  def __init__(self, data_string):
    parts = data_string.strip().split(';')
    self.factor_type = parts[0]
    self.output_rv = int(parts[1])
    self.input_rvs = []
    for p in parts[2:]:
      self.input_rvs.append(int(p))
    if self.factor_type == 'AND' and len(self.input_rvs) < 2:
      print('Warning: AND factors has %d inputs' % len(self.input_rvs))

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

  def f(rvs):
    # TODO: formulate this as vectorized operations
    err = 0.0
    for i in range(n):
      factor = factors[i]
      if factor.factor_type == 'SAME':
        inp, out = factor.input_rvs[0], factor.output_rv
        err += (rvs[inp] - rvs[out]) ** 2.0
      elif factor.factor_type == 'INV':
        inp, out = factor.input_rvs[0], factor.output_rv
        err += (1.0 - rvs[inp] - rvs[out]) ** 2.0
      if factor.factor_type == 'AND':
        inp1, inp2 = factor.input_rvs
        out = factor.output_rv
        err += (rvs[inp1] * rvs[inp2] - rvs[out]) ** 2.0
    return err

  lower = np.zeros(n)
  upper = np.ones(n)
  for rv, val in observed.items():
    lower[rv] = float(val)
    upper[rv] = float(val)
  bounds = Bounds(lower, upper)

  same, inv = np.zeros((n, n)), np.zeros((n, n))
  inv_b = np.zeros(n)

  for i in range(n):
    factor = factors[i]
    if factor.factor_type == 'SAME':
      inp, out = factor.input_rvs[0], factor.output_rv
      same[out, out] = 1.0
      same[out, inp] = -1.0
    elif factor.factor_type == 'INV':
      inp, out = factor.input_rvs[0], factor.output_rv
      inv[out, out] = 1.0
      inv[out, inp] = 1.0
      inv_b[out] = 1.0

  def same_constraint(x):
    return same.dot(x.reshape((-1, 1))).squeeze()

  def inv_constraint(x):
    return inv_b - inv.dot(x.reshape((-1, 1))).squeeze()

  cons = [
    # {'type': 'eq', 'fun': same_constraint},
    # {'type': 'eq', 'fun': inv_constraint}
  ]

  options = {
    'maxiter': 40,
    'disp': True
  }

  start = time()
  print('Starting optimization')
  result = minimize(f, init_guess, constraints=cons,
                    bounds=bounds, options=options)
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

def main(test_case):
  base = os.path.abspath('data')
  dataset_dir = os.path.join(base, test_case)
  config_file = os.path.join(dataset_dir, 'params.yaml')
  factor_file = os.path.join(dataset_dir, 'factors.txt')
  data_file = os.path.join(dataset_dir, 'data.bits')

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
    true_input      = BitVector(size=len(input_rvs))
    for rv in input_rvs:
      predicted_input[rv] = bool(predictions[rv])
      true_input[rv] = sample[rv]

    verify(true_input, predicted_input, config)


if __name__ == '__main__':
  # main('sha256')
  main('lossyPseudoHash')
  # main('xorConst')
  print('Done.')
  sys.exit(0)
