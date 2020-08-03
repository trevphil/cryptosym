# -*- coding: utf-8 -*-
import os
import sys
import random
import argparse
import yaml
import numpy as np
import networkx as nx
from pathlib import Path
from BitVector import BitVector
from matplotlib import pyplot as plt

from dataset_generation import pseudo_hashes
from dataset_generation.factor import Factor


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
    'sha256': pseudo_hashes.SHA256Hash(),
    'lossyPseudoHash': pseudo_hashes.LossyPseudoHash(),
    'nonLossyPseudoHash': pseudo_hashes.NonLossyPseudoHash(),
    'xorConst': pseudo_hashes.XorConst(),
    'shiftLeft': pseudo_hashes.ShiftLeft(),
    'shiftRight': pseudo_hashes.ShiftRight(),
    'invert': pseudo_hashes.Invert(),
    'andConst': pseudo_hashes.AndConst(),
    'orConst': pseudo_hashes.OrConst(),
    'addConst': pseudo_hashes.AddConst(),
    'add': pseudo_hashes.Add(),
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
  parser.add_argument('--visualize', action='store_true',
                      help='Visualize the symbolic graph of bit dependencies')
  parser.add_argument('--hash-input', type=str, default=None,
                      help='Give input message in hex to simply print the hash of the input')
  args = parser.parse_args()

  if args.hash_input is not None:
    print('Computing {} hash for: {}\n'.format(args.hash_algo, args.hash_input))
    num = int(args.hash_input, 16)
    input_bv = BitVector(intVal=num, size=args.num_input_bits)
    algo = hash_algos[args.hash_algo]
    algo(input_bv, difficulty=args.difficulty)
    hash_indices = algo.hash_rv_indices
    hash_output = BitVector(size=len(hash_indices))
    bitvec = algo.allBits()
    for i, bit_index in enumerate(hash_indices):
      hash_output[i] = bitvec[bit_index]
    print(hex(int(hash_output))[2:])
    return

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
  input_indices = algo.input_rv_indices
  hash_indices = algo.hash_rv_indices

  print('Saving hash function symbolically as a factor graph...')
  algo.saveFactors(graph_file)

  print('Allocating data...')
  data = np.zeros((N, n), dtype=bool)
  example_sample = None

  print('Populating data...')
  for sample_idx in range(N):
    hash_input = sample(num_input_bits)
    algo(hash_input, difficulty=args.difficulty)
    bitvec = algo.allBits()
    if example_sample is None:
      example_sample = hex(int(bitvec))[2:]
    # The MSB (left-most) of the BitVector will go in the left-most column of `data`
    data[N - sample_idx - 1, :] = bitvec

  print('Transposing matrix and converting back to BitVector...')
  bv = BitVector(bitlist=data.T.reshape((1, -1)).squeeze().tolist())

  print('Saving samples to %s' % data_file)
  with open(data_file, 'wb') as f:
    bv.write_to_file(f)

  params = {
    'hash': args.hash_algo,
    'num_rvs': n,
    'num_samples': N,
    'input_rv_indices': input_indices,
    'hash_rv_indices': hash_indices,
    'example_sample': example_sample
  }
  
  if algo == 'sha256':
    params.update({'difficulty': args.difficulty})

  with open(params_file, 'w') as f:
    yaml.dump(params, f, default_flow_style=None)

  print('Generated dataset with {} samples (hash={}, {} input bits).'.format(
    N, args.hash_algo, num_input_bits))
  
  if args.visualize:
    nx.draw(Factor.directed_graph, node_size=50, with_labels=True)
    plt.show()


if __name__ == '__main__':
  main()
  sys.exit(0)
