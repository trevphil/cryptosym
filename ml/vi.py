# -*- coding: utf-8 -*-
#!/usr/bin/python3

import os
import sys
from time import time
import yaml
import arviz as az
import matplotlib.pyplot as plt
import numpy as np
import pymc3 as pm
import theano
from BitVector import BitVector

DATASET_DIR = os.path.abspath('data/nonLossyPseudoHash')
DATA_FILE   = os.path.join(DATASET_DIR, 'data.bits')
FACTOR_FILE = os.path.join(DATASET_DIR, 'factors.txt')
PARAMS_FILE = os.path.join(DATASET_DIR, 'params.yaml')

NUM_SAMPLES = 10000
EPS = 1e-6
PRIOR = np.array([0.5, 0.5])
XOR   = np.array([[[1-EPS, EPS],
                   [EPS, 1-EPS]],
                  [[EPS, 1-EPS],
                   [1-EPS, EPS]]])
AND   = np.array([[[1-EPS, EPS],
                   [1-EPS, EPS]],
                  [[1-EPS, EPS],
                   [0, 1]]])
OR    = np.array([[[1-EPS, EPS],
                   [EPS, 1-EPS]],
                  [[EPS, 1-EPS],
                   [EPS, 1-EPS]]])
INV   = np.array([[EPS, 1-EPS],
                  [1-EPS, EPS]])

PRIOR = theano.shared(PRIOR)
XOR = theano.shared(XOR)
AND = theano.shared(AND)
OR = theano.shared(OR)
INV = theano.shared(INV)


class Factor(object):
  def __init__(self, data_string):
    parts = data_string.strip().split(';')
    self.factor_type = parts[0]
    self.output_rv = int(parts[1])
    self.input_rvs = set()
    for p in parts[2:]:
      self.input_rvs.add(int(p))


def run_trial(factors, params, observed):
  n = int(params['num_rvs'])
  N = int(params['num_samples'])
  input_rvs = set(params['input_rv_indices'])
  hash_rvs = set(params['hash_rv_indices'])
  difficulty = int(params['difficulty'])
  hash_algo = params['hash']

  rvs = dict()

  start = time()
  print('Setting up PyMC3 model...')

  for rv_idx in range(n):
    factor = factors[rv_idx]
    f_type = factor.factor_type
    num_inputs = len(factor.input_rvs)

    if num_inputs == 0:
      rvs[rv_idx] = pm.Categorical(str(rv_idx), p=PRIOR)
    elif num_inputs == 1:
      in1 = [inp for inp in factor.input_rvs][0]
      rvs[rv_idx] = ~rvs[in1]
    elif num_inputs == 2:
      in1, in2 = [inp for inp in factor.input_rvs]
      if f_type == 'XOR':
        rvs[rv_idx] = rvs[in1] ^ rvs[in2]
      elif f_type == 'AND':
        rvs[rv_idx] = rvs[in1] & rvs[in2]
      elif f_type == 'OR':
        rvs[rv_idx] = rvs[in1] | rvs[in2]

    observed_val = observed.get(rv_idx, None)
    if observed_val is not None:
      rvs[rv_idx].observed = int(observed_val)

  print('Finished setting up model in %.2f seconds' % (time() - start))

  # trace = pm.sample(NUM_SAMPLES)
  mean_field = pm.fit(method='advi')
  trace = mean_field.sample(NUM_SAMPLES)
  plt.plot(mean_field.hist)
  plt.show()

  return trace


if __name__ == '__main__':
  factors = dict()
  with open(FACTOR_FILE, 'r') as f:
    for line in f:
      factor = Factor(line)
      factors[factor.output_rv] = factor

  with open(PARAMS_FILE, 'r') as f:
    params = yaml.safe_load(f)

  n = int(params['num_rvs'])
  N = int(params['num_samples'])
  hash_rvs = set(params['hash_rv_indices'])

  for i in range(n):
    if factors.get(i, None) is None:
      factors[i] = Factor('PRIOR;{}'.format(i))

  print('{} random variables, {} factors'.format(n, len(factors)))

  bv = BitVector(filename=DATA_FILE)
  data = bv.read_bits_from_file(n * N)
  bv.close_file_object()
  data = np.array([bit for bit in data], dtype=bool)
  data = data.reshape((n, N)).T  # data is (N x n)

  for sample_idx in range(N):
    print('Test case {}/{}'.format(sample_idx + 1, N))
    sample = data[sample_idx, :]

    observed = dict()
    for rv_idx in hash_rvs:
      observed[rv_idx] = sample[rv_idx]

    with pm.Model() as model:
      predictions = run_trial(factors, params, observed)

  sys.exit(0)
