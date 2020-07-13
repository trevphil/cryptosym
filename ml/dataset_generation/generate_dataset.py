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

from dataset_generation import pseudo_hashes


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


def hashAlgos():
  return {
    'pseudoHash': pseudo_hashes.PseudoHash(),
    'xorHashConst': pseudo_hashes.XorHashConst(),
    'shiftLeft': pseudo_hashes.ShiftLeft(),
    'shiftRight': pseudo_hashes.ShiftRight()
  }


def main():
  random.seed(0)
  np.random.seed(0)
  
  hash_algos = hashAlgos()

  parser = argparse.ArgumentParser(description='Hash reversal dataset generator')
  parser.add_argument('--data-dir', type=str, default=os.path.abspath('./data'),
                      help='Path to the directory where the dataset should be stored')
  parser.add_argument('--num-samples', type=int, default=10,
                      help='Number of samples to use in the dataset')
  parser.add_argument('--num-input-bits', type=int, default=64,
                      help='Number of bits in each input message to the hash function')
  parser.add_argument('--hash-algo', type=str, default='sha256',
                      choices=list(sorted(hashAlgos().keys())),
                      help='Choose the hashing algorithm to apply to the input data')
  parser.add_argument('--difficulty', type=int, default=64,
                      help='SHA-256 difficulty (an interger between 1 and 64 inclusive)')
  args = parser.parse_args()

  num_input_bits = args.num_input_bits
  N = int(8 * round(args.num_samples / 8))  # number of samples

  dataset_dir = os.path.join(args.data_dir, args.hash_algo)
  data_file = os.path.join(dataset_dir, 'data.bits')
  params_file = os.path.join(dataset_dir, 'params.yaml')
  graph_file = os.path.join(dataset_dir, 'factors.txt')

  Path(dataset_dir).mkdir(parents=True, exist_ok=True)

  for f in [data_file, params_file, params_file]:
    if os.path.exists(f):
      os.remove(f)

  # Generate symbolic dependencies as factors
  algo = hash_algos[args.hash_algo]
  algo(sample(num_input_bits), difficulty=args.difficulty)
  n = algo.numVars()

  print('Saving hash function symbolically as a factor graph...')
  algo.saveFactors(graph_file)

  print('Allocating data...')
  data = BitVector(size=n * N)

  print('Populating data...')
  for sample_idx in tqdm(range(N)):
    i = sample_idx * n
    hash_input = sample(num_input_bits)
    algo(hash_input, difficulty=args.difficulty)
    data[i:i + n] = algo.bits()

  print('Converting BitVector to numpy matrix...')
  data = np.array(data, dtype=bool).reshape((N, n))

  print('Transposing matrix and converting back to BitVector...')
  bv = BitVector(bitlist=data.T.reshape((1, -1)).squeeze().tolist())

  print('Saving samples to %s' % data_file)
  with open(data_file, 'wb') as f:
    bv.write_to_file(f)

  params = {
    'hash': args.hash_algo,
    'num_rvs': n,
    'num_samples': N,
    'num_hash_bits': algo.numHashBits(),
    'num_input_bits': num_input_bits,
    'num_internal_bits': n - num_input_bits - algo.numHashBits()
  }
  
  if algo == 'sha256':
    params.update({'difficulty': args.difficulty})

  with open(params_file, 'w') as f:
    yaml.dump(params, f)

  print('Generated dataset with {} samples (hash={}, {} input bits).'.format(
    N, args.hash_algo, num_input_bits))


if __name__ == '__main__':
  main()
  sys.exit(0)
