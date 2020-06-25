# -*- coding: utf-8 -*-
import os
import sys
import random
import argparse
import numpy as np
from pathlib import Path
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
  return BitVector(intVal=0).gen_random_bits(nbits)


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
  parser.add_argument('--hash-algo', type=str, default='sha256',
                      choices=['sha256', 'md5', 'map_from_input', 'conditioned_on_input_and_hash', 'pseudo_hash'],
                      help='Choose the hashing algorithm to apply to the input data')
  args = parser.parse_args()

  algo = args.hash_algo
  num_input_bits = args.num_input_bits
  N = args.num_samples  # number of samples
  tmp1, tmp2 = hashFunc(sample(num_input_bits), algo)
  n = (tmp1 + tmp2).length() + num_input_bits  # number of variables

  data_file = '{}-{}-{}.bits'.format(algo, n, N)
  data_file = os.path.join(args.data_dir, data_file)

  Path(args.data_dir).mkdir(parents=True, exist_ok=True)

  if os.path.exists(data_file):
    os.remove(data_file)

  print('Allocating data matrix...')
  data = np.zeros((n, N), dtype=bool)

  print('Populating data matrix...')
  for sample_idx in range(N):
    hash_input = sample(num_input_bits)
    hash_output, internals = hashFunc(hash_input, algo)
    data[:, sample_idx] = hash_output + hash_input + internals  # BitVectors will be concatenated

  print('Linearizing data to a BitVector...')
  bv = BitVector(bitlist=data.reshape((1, -1)).squeeze().tolist())

  print('Saving to %s' % data_file)
  with open(data_file, 'wb') as f:
    bv.write_to_file(f)

  print('Generated dataset with %d samples (hash=%s, %d input bits).' % (N, algo, num_input_bits))


if __name__ == '__main__':
  main()
  sys.exit(0)
