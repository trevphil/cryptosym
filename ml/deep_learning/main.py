# -*- coding: utf-8 -*-

import os
import sys
import yaml
import torch
import random
import argparse
import numpy as np
from torch import optim
from BitVector import BitVector
from collections import defaultdict

from deep_learning.factor import Factor
from deep_learning.models import ReverseHash
from deep_learning.loss import Loss


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
    n = int(config['num_bits_per_sample'])
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


def main(dataset):
    random.seed(1)
    np.random.seed(1)
    torch.manual_seed(1)

    config_file = os.path.join(dataset, 'params.yaml')
    factor_file = os.path.join(dataset, 'factors.txt')
    data_file = os.path.join(dataset, 'data.bits')

    config = load_config(config_file)
    factors = load_factors(factor_file)
    bitvectors = load_bitvectors(data_file, config)

    n = int(config['num_bits_per_sample'])
    n_input = int(config['num_input_bits'])
    N = int(config['num_samples'])
    obs_rv_set = set(config['observed_rv_indices'])
    obs_indices = np.array(sorted(obs_rv_set), dtype=int)
    obs_rv2idx = {rv: i for i, rv in enumerate(sorted(obs_indices))}
    num_obs = len(obs_rv_set)

    parents_per_rv = defaultdict(lambda: 0)
    for _, factor in factors.items():
        for input_rv in factor.input_rvs:
            parents_per_rv[input_rv] += 1

    N_train = int(N * 0.70)
    N_val = int(N * 0.15)
    N_test  = N - N_train - N_val
    num_epochs = 5

    model = ReverseHash(factors, obs_rv_set, obs_rv2idx,
                        n_input, parents_per_rv)
    loss = Loss(factors, obs_rv_set, obs_rv2idx, n_input)
    optimizer = optim.Adam(model.parameters(), lr=0.0001)

    for epoch in range(num_epochs):
        print('Starting epoch %d' % epoch)

        # Train
        print('\tStarting training')
        model.train()
        for sample in bitvectors[:N_train]:
            sample = torch.tensor(sample)
            optimizer.zero_grad()
            model_input = torch.zeros(num_obs)
            for i, rv in enumerate(obs_indices):
                model_input[i] = sample[rv]
            model_input = torch.reshape(model_input, (1, num_obs))
            predicted_input = model(model_input)
            target_hash = torch.zeros(len(obs_rv_set))
            for rv, i in obs_rv2idx.items():
                target_hash[i] = sample[rv]
            total_loss = loss(predicted_input, target_hash)
            total_loss.backward()
            optimizer.step()

        # Validation
        print('\tStarting validation')
        model.eval()
        cum_loss = 0.0
        for sample in bitvectors[N_train:N_train + N_val]:
            sample = torch.tensor(sample)
            model_input = torch.zeros(num_obs)
            for i, rv in enumerate(obs_indices):
                model_input[i] = sample[rv]
            model_input = torch.reshape(model_input, (1, num_obs))
            predicted_input = model(model_input)
            target_hash = torch.zeros(len(obs_rv_set))
            for rv, i in obs_rv2idx.items():
                target_hash[i] = sample[rv]
            cum_loss += loss(predicted_input, target_hash)
        print('\tAverage validation loss is %.3f' % (cum_loss / N_val))

    # Test
    print('Starting evaluation on test dataset')
    model.eval()
    cum_loss = 0.0
    for sample in bitvectors[N_train + N_val:]:
        sample = torch.tensor(sample)
        model_input = torch.zeros(num_obs)
        for i, rv in enumerate(obs_indices):
            model_input[i] = sample[rv]
        model_input = torch.reshape(model_input, (1, num_obs))
        predicted_input = model(model_input)
        target_hash = torch.zeros(len(obs_rv_set))
        for rv, i in obs_rv2idx.items():
            target_hash[i] = sample[rv]
        cum_loss += loss(predicted_input, target_hash)
    print('\tAverage test loss is %.3f' % (cum_loss / N_test))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Hash reversal via deep learning')
    parser.add_argument(
        'dataset',
        type=str,
        help='Path to the dataset directory')
    args = parser.parse_args()
    main(args.dataset)
    print('Done.')
    sys.exit(0)
