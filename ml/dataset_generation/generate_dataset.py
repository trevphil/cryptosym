# -*- coding: utf-8 -*-
import os
import sys
import random
import argparse
import yaml
import numpy as np
from pathlib import Path
from tqdm import tqdm
from BitVector import BitVector

from dataset_generation.hash_functions import hashFunc

"""
This generates a binary file representation of the dataset.
Each dataset "sample" is 256 hash bits followed by the hash input bits
of fixed length, followed by (optional) additional bits of fixed length
from computations inside of the hashing algorithm.

The number of "random variables" is n = (256 + num_input_bits + num_internals)
and the number of samples is N. Data is ordered such that each row is a
random variable, and each column is a sample.

The data is written to a file as a single bit vector by concatenating rows.
The file format is `<HASH_ALGO>-<NUM_RANDOM_VARIABLES>-<NUM_SAMPLES>.bits`.
"""


def sample(nbits):
  """ Returns a BitVector with `nbits` initialized with a random value """
  num = random.randint(0, (2 ** nbits) - 1)
  return BitVector(intVal=num, size=nbits)


def main():
  random.seed(0)
  np.random.seed(0)

  parser = argparse.ArgumentParser(description='Hash reversal dataset generator')
  parser.add_argument('--data-dir', type=str, default=os.path.abspath('./data'),
                      help='Path to the directory where the dataset should be stored')
  parser.add_argument('--num-samples', type=int, default=20000,
                      help='Number of samples to use in the dataset')
  parser.add_argument('--num-input-bits', type=int, default=64,
                      help='Number of bits in each input message to the hash function')
  parser.add_argument('--pct-train', type=float, default=0.8,
                      help='Percent of samples which will be used in the "train" section of the dataset')
  parser.add_argument('--hash-algo', type=str, default='sha256',
                      choices=['sha256', 'md5', 'map_from_input', 'conditioned_on_input_and_hash', 'pseudo_hash'],
                      help='Choose the hashing algorithm to apply to the input data')
  parser.add_argument('--difficulty', type=int, default=64,
                      help='SHA-256 difficulty (an interger between 1 and 64 inclusive)')
  args = parser.parse_args()

  algo = args.hash_algo
  num_input_bits = args.num_input_bits
  N = int(8 * round(args.num_samples / 8))  # number of samples
  N_train = int(8 * round((N * args.pct_train) / 8))
  N_test = N - N_train
  tmp1, tmp2 = hashFunc(sample(num_input_bits), algo, args.difficulty)
  n = (tmp1 + tmp2).length() + num_input_bits  # number of variables

  dataset_dir = '{}-{}-{}-{}'.format(algo, n, N, num_input_bits)
  dataset_dir = os.path.join(args.data_dir, dataset_dir)
  data_file = os.path.join(dataset_dir, 'data.bits')
  params_file = os.path.join(dataset_dir, 'params.yaml')

  Path(dataset_dir).mkdir(parents=True, exist_ok=True)

  if os.path.exists(data_file):
    os.remove(data_file)

  print('Allocating data...')
  data = BitVector(size=n * N)

  print('Populating data...')
  for sample_idx in tqdm(range(N)):
    i = sample_idx * n
    hash_input = sample(num_input_bits)
    hash_output, internals = hashFunc(hash_input, algo, args.difficulty)
    j = len(hash_output)
    data[i:i + j] = hash_output
    i += j
    j = len(hash_input)
    data[i:i + j] = hash_input
    i += j
    j = len(internals)
    data[i:i + j] = internals

  print('Converting BitVector to numpy matrix...')
  data = np.array(data, dtype=bool).reshape((N, n))

  print('Transposing matrix and converting back to BitVector...')
  bv = BitVector(bitlist=data.T.reshape((1, -1)).squeeze().tolist())

  print('Saving to %s' % data_file)
  with open(data_file, 'wb') as f:
    bv.write_to_file(f)

  params = {
    'hash': algo,
    'num_rvs': n,
    'num_samples': N,
    'num_train_samples': N_train,
    'num_test_samples': N_test,
    'num_hash_bits': tmp1.length(),
    'num_input_bits': num_input_bits,
    'num_internal_bits': tmp2.length()
  }
  
  if algo == 'sha256':
    params.update({'difficulty': args.difficulty})

  with open(params_file, 'w') as f:
    yaml.dump(params, f)

  print('Generated dataset with %d samples (hash=%s, %d input bits).' % (N, algo, num_input_bits))


if __name__ == '__main__':
  main()
  sys.exit(0)
