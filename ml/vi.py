# -*- coding: utf-8 -*-
#!/usr/bin/python3

import os
import sys
from time import time
import subprocess
import yaml
from tqdm import tqdm
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
X = lambda i: 'X' + str(i)

STD = 0.0001
NUM_SAMPLES = 10000


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

  rvs = dict()

  start = time()
  print('Setting up PyMC3 model...')

  for rv_idx in tqdm(range(n)):
    factor = factors[rv_idx]
    f_type = factor.factor_type
    num_inputs = len(factor.input_rvs)
    
    observed_val = observed.get(rv_idx, None)
    if observed_val is not None:
      observed_val = int(observed_val)

    if num_inputs == 0:
      rvs[rv_idx] = pm.Normal(X(rv_idx), 0.5, STD, observed=observed_val)
    elif num_inputs == 1:
      in1_idx = [inp for inp in factor.input_rvs][0]
      rvs[rv_idx] = pm.Normal(X(rv_idx), 1.0 - rvs[in1_idx], STD, observed=observed_val)
    elif num_inputs == 2:
      in1_idx, in2_idx = [inp for inp in factor.input_rvs]
      if f_type == 'AND':
        rvs[rv_idx] = pm.Normal(X(rv_idx), rvs[in1_idx] * rvs[in2_idx], STD, observed=observed_val)
      else:
        raise RuntimeError('Unsupported factor: {}'.format(f_type))
    else:
      raise RuntimeError('Factor has {} inputs'.format(num_inputs))

  print('Finished setting up model in %.2f seconds' % (time() - start))

  start = time()
  print('Fitting model with variational inference...')
  mean_field = pm.fit(method='advi')
  print('Finished fitting model in %.2f seconds' % (time() - start))

    # See https://docs.pymc.io/notebooks/variational_api_quickstart.html
  print('Sampling %d samples...' % NUM_SAMPLES)
  trace = mean_field.sample(NUM_SAMPLES)
  # plt.plot(mean_field.hist)
  # plt.show()

  summary = pm.summary(trace)
  print(summary)
  print(summary.shape)
  return summary


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
  input_rvs = list(sorted(params['input_rv_indices']))
  hash_rvs = set(params['hash_rv_indices'])
  hash_algo = params['hash']
  difficulty = params['difficulty']

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
    sample = data[N - sample_idx - 1, :]
    true_input = BitVector(size=len(input_rvs))
    for rv in input_rvs:
      true_input[rv] = sample[rv]

    predicted_input = BitVector(size=len(input_rvs))
    observed = {rv: sample[rv] for rv in hash_rvs}

    with pm.Model() as model:
      predictions = run_trial(factors, params, observed)

    for rv_idx in input_rvs:
      prob_one = observed.get(rv_idx, None)
      if prob_one is None:
        prob_one = predictions['mean'][X(rv_idx)]
      predicted_input[rv_idx] = 1 if prob_one > 0.5 else 0
    
    true_in = hex(int(true_input))[2:]
    pred_in = hex(int(predicted_input))[2:]
    
    print('Hash input: {}'.format(true_in))
    print('Pred input: {}'.format(pred_in))

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
      print('Expected {}, got {}'.format(true_out, pred_out))

  sys.exit(0)
