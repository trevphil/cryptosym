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


class GNC(object):
  def __init__(self, c):
    self._c_sq = c * c
    self._mu = None
    self._itr = 0

  def mu(self, err_vector):
    if self._mu is None:
      self._mu = 2 * np.max(err_vector) / self._c_sq
      print('mu initialized to %.3f' % self._mu)
    return max(1.0, self._mu)

  def iteration(self):
    return self._itr

  def increment(self):
    self._itr += 1
    if self._mu is not None:
      self._mu = max(1.0, self._mu / 1.4)
      print('mu is now %.3f' % self._mu)


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

  """
  (i - j) ^ 2 = i^2 - 2ij + j^2
    d/di = 2i - 2j
    d/dj = 2j - 2i

  (1 - i - j) ^ 2
    d/di = 2i + 2j - 2
    d/dj = 2j + 2i - 2

  (ij - k) ^ 2
    d/di = 2ij^2 - 2jk
    d/dj = 2i^2j - 2ik
    d/dk = 2k - 2ij
  """

  def partial(self, with_respect_to, x):
    wrt = with_respect_to
    if wrt != self.output_rv and wrt not in self.input_rvs:
      raise RuntimeError

    if self.factor_type == 'SAME':
      i, j = self.input_rvs[0], self.output_rv
      if wrt == i:
        return 2 * (x[i] - x[j])
      else:
        return 2 * (x[j] - x[i])
    elif self.factor_type == 'INV':
      i, j = self.input_rvs[0], self.output_rv
      return 2 * (x[i] + x[j] - 1)
    elif self.factor_type == 'AND':
      i, j = self.input_rvs
      k = self.output_rv
      if wrt == i:
        return 2 * (x[i] * x[j] * x[j] - x[j] * x[k])
      elif wrt == j:
        return 2 * (x[i] * x[i] * x[j] - x[i] * x[k])
      else:
        return 2 * (x[k] - x[i] * x[j])
    else:
      return 0.0


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

  # Jacobian: J[i] = derivative of f(x) w.r.t. variable `i`
  def jacobian(x):
    J = np.zeros(n)
    for i in range(n):
      for factor_idx in factors_per_rv[i]:
        J[i] += factors[factor_idx].partial(i, x)
    return J

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

  lower, upper = np.zeros(n), np.ones(n)
  for rv, val in observed.items():
    lower[rv] = float(val)
    upper[rv] = float(val)

  options = {'maxiter': 40, 'disp': False}

  start = time()
  print('Starting optimization...')
  result = minimize(f, init_guess, bounds=Bounds(lower, upper),
                    jac=jacobian, options=options, callback=cb)
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
  main('lossyPseudoHash')
  print('Done.')
  sys.exit(0)
