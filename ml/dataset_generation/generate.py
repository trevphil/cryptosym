# -*- coding: utf-8 -*-
from dataset_generation.factor import Factor
from dataset_generation import hash_funcs

import os
import sys
import random
import argparse
import h5py
import yaml
import pydot
import numpy as np
import networkx as nx
from pathlib import Path
from BitVector import BitVector
from matplotlib import pyplot as plt
from networkx.drawing.nx_pydot import graphviz_layout

plt.rcParams['figure.figsize'] = (15, 5)


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


def extend(dataset, data):
    dataset.resize(dataset.shape[0] + 1, axis=0)
    dataset[-1] = data


def main():
    random.seed(0)
    np.random.seed(0)

    hash_algos = hashAlgos()

    parser = argparse.ArgumentParser(
        description='Hash reversal dataset generator')
    parser.add_argument('--data-dir', type=str, default=os.path.abspath('./data'),
                        help='Path to the directory where the dataset should be stored')
    parser.add_argument('--num-samples', type=int, default=64,
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
    parser.add_argument('--pct-val', type=float, default=0.15,
                        help='Percent of samples used for validation dataset')
    parser.add_argument('--pct-test', type=float, default=0.15,
                        help='Percent of samples used for test dataset')
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
    N_val = max(1, int(round(N * args.pct_val)))
    N_test = max(1, int(round(N * args.pct_test)))
    N_train = N - N_val - N_test

    algo_dir = '{}_d{}'.format(args.hash_algo, args.difficulty)
    dataset_dir = os.path.join(args.data_dir, algo_dir)
    data_file = os.path.join(dataset_dir, 'data.bits')
    params_file = os.path.join(dataset_dir, 'params.yaml')
    graph_file = os.path.join(dataset_dir, 'factors.txt')
    viz_file = os.path.join(dataset_dir, 'graph.pdf')
    train_file = os.path.join(dataset_dir, 'train.hdf5')
    val_file = os.path.join(dataset_dir, 'val.hdf5')
    test_file = os.path.join(dataset_dir, 'test.hdf5')

    Path(dataset_dir).mkdir(parents=True, exist_ok=True)

    for f in [data_file, params_file, graph_file, viz_file,
        train_file, val_file, test_file]:
        if os.path.exists(f):
            os.remove(f)

    # Generate symbolic dependencies as factors
    algo = hash_algos[args.hash_algo]
    algo(sample(num_input_bits), difficulty=args.difficulty)
    n = algo.bitsPerSample()
    hash_rv_indices = algo.hashIndices()
    num_useful_factors = algo.numUsefulFactors()
    n_target = len(hash_rv_indices)

    print('Saving hash function symbolically as a factor graph...')
    algo.saveFactors(graph_file)

    datasets = {
        'train': (h5py.File(train_file, 'w'), N_train),
        'val': (h5py.File(val_file, 'w'), N_val),
        'test': (h5py.File(test_file, 'w'), N_test)
    }

    bv = BitVector(size=0)
    for split in ['train', 'val', 'test']:
        print('Creating %s split...' % split)
        dset, num_samples = datasets[split]
        bits = dset.create_dataset('bits', (0, n),
            maxshape=(num_samples, n), dtype=bool)
        target = dset.create_dataset('target', (0, n_target),
            maxshape=(num_samples, n_target), dtype=bool)
        for sample_idx in range(num_samples):
            hash_input = sample(num_input_bits)
            algo(hash_input, difficulty=args.difficulty)
            bitvals = algo.allBits()
            bv += bitvals
            targetvals = algo.forwardPropagateInput(hash_input)
            if len(bv) % 8 == 0:
                # Wait until the BitVector is a multiple of 8
                with open(data_file, 'ab') as bin_file:
                    bv.write_to_file(bin_file)
                bv = BitVector(size=0)  # Reset the BitVector
            extend(bits, np.array(bitvals, dtype=bool))
            extend(target, np.array(targetvals, dtype=bool))
        dset.close()
    assert len(bv) == 0, 'Data is garbage, not a multiple of 8'

    params = {
        'hash': args.hash_algo,
        'num_input_bits': num_input_bits,
        'num_bits_per_sample': n,
        'num_useful_factors': num_useful_factors,
        'num_samples': N,
        'num_train': N_train,
        'num_val': N_val,
        'num_test': N_test,
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
