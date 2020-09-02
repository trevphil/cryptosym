# -*- coding: utf-8 -*-
from dataset_generation.factor import Factor
from dataset_generation import hash_funcs
import os
import sys
import random
import argparse
import yaml
import pydot
import numpy as np
import networkx as nx
from pathlib import Path
from BitVector import BitVector
from matplotlib import pyplot as plt
from networkx.drawing.nx_pydot import graphviz_layout

plt.rcParams['figure.figsize'] = (15, 5)


"""
TODO: Update this documentation

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
        'sha256': hash_funcs.SHA256Hash(),
        'lossyPseudoHash': hash_funcs.LossyPseudoHash(),
        'nonLossyPseudoHash': hash_funcs.NonLossyPseudoHash(),
        'xorConst': hash_funcs.XorConst(),
        'shiftLeft': hash_funcs.ShiftLeft(),
        'shiftRight': hash_funcs.ShiftRight(),
        'invert': hash_funcs.Invert(),
        'andConst': hash_funcs.AndConst(),
        'orConst': hash_funcs.OrConst(),
        'addConst': hash_funcs.AddConst(),
        'add': hash_funcs.Add(),
    }


def main():
    random.seed(0)
    np.random.seed(0)

    hash_algos = hashAlgos()

    parser = argparse.ArgumentParser(
        description='Hash reversal dataset generator')
    parser.add_argument('--data-dir', type=str, default=os.path.abspath('./data'),
                        help='Path to the directory where the dataset should be stored')
    parser.add_argument('--num-samples', type=int, default=10,
                        help='Number of samples to use in the dataset')
    parser.add_argument('--num-input-bits', type=int, default=64,
                        help='Number of bits in each input message to the hash function')
    parser.add_argument('--hash-algo', type=str, default='sha256',
                        choices=list(sorted(hashAlgos().keys())),
                        help='Choose the hashing algorithm to apply to the input data')
    parser.add_argument('--difficulty', type=int, default=1,
                        help='SHA-256 difficulty (an interger between 1 and 64 inclusive)')
    parser.add_argument('--visualize', action='store_true',
                        help='Visualize the symbolic graph of bit dependencies')
    parser.add_argument('--hash-input', type=str, default=None,
                        help='Give input message in hex to simply print the hash of the input')
    args = parser.parse_args()

    if args.hash_input is not None:
        num = int(args.hash_input, 16)
        input_bv = BitVector(intVal=num, size=args.num_input_bits)
        algo = hash_algos[args.hash_algo]
        hash_output = algo(input_bv, difficulty=args.difficulty)
        digest_str = hex(int(hash_output))[2:]
        pad = '0' * (len(hash_output) // 4 - len(digest_str))
        print(pad + digest_str, end='')
        return

    num_input_bits = args.num_input_bits
    N = int(8 * round(args.num_samples / 8))  # number of samples
    N = max(N, 8)

    dataset_dir = os.path.join(args.data_dir, args.hash_algo)
    data_file = os.path.join(dataset_dir, 'data.bits')
    params_file = os.path.join(dataset_dir, 'params.yaml')
    graph_file = os.path.join(dataset_dir, 'factors.txt')
    viz_file = os.path.join(dataset_dir, 'graph.pdf')

    Path(dataset_dir).mkdir(parents=True, exist_ok=True)

    for f in [data_file, params_file, graph_file, viz_file]:
        if os.path.exists(f):
            os.remove(f)

    # Generate symbolic dependencies as factors
    algo = hash_algos[args.hash_algo]
    algo(sample(num_input_bits), difficulty=args.difficulty)
    n = algo.bitsPerSample()
    hash_rv_indices = algo.hashIndices()
    num_useful_factors = algo.numUsefulFactors()

    print('Saving hash function symbolically as a factor graph...')
    algo.saveFactors(graph_file)

    print('Allocating data...')
    data = np.zeros((N, n), dtype=bool)

    print('Populating data...')
    for sample_idx in range(N):
        hash_input = sample(num_input_bits)
        algo(hash_input, difficulty=args.difficulty)
        # The MSB (left-most) of the BitVector will go in the left-most column
        # of `data`
        data[N - sample_idx - 1, :] = algo.allBits()

    print('Transposing matrix and converting back to BitVector...')
    bv = BitVector(bitlist=data.T.reshape((1, -1)).squeeze().tolist())

    print('Saving samples to %s' % data_file)
    with open(data_file, 'wb') as f:
        bv.write_to_file(f)

    params = {
        'hash': args.hash_algo,
        'num_input_bits': num_input_bits,
        'num_bits_per_sample': n,
        'num_useful_factors': num_useful_factors,
        'num_samples': N,
        'observed_rv_indices': hash_rv_indices,
        'difficulty': args.difficulty
    }

    with open(params_file, 'w') as f:
        yaml.dump(params, f, default_flow_style=None)

    print('Generated dataset with parameters:')
    for key in sorted(params.keys()):
        if key != 'observed_rv_indices':
            print('\t{}: {}'.format(key, params[key]))

    cyclic = len(list(nx.simple_cycles(Factor.directed_graph))) > 0
    print('Directed graph cyclic? {}'.format(cyclic))

    if args.visualize:
        colors = ['#0000ff' for _ in range(n)]
        for input_idx in range(num_input_bits):
            colors[input_idx] = '#000000'
        for output_idx in hash_rv_indices:
            colors[output_idx] = '#00ff00'
        pos = graphviz_layout(Factor.directed_graph, prog='dot')
        nx.draw(Factor.directed_graph, pos, node_size=5,
                with_labels=False, node_color=colors, width=0.25, arrowsize=4)
        plt.savefig(viz_file)


if __name__ == '__main__':
    main()
    sys.exit(0)
