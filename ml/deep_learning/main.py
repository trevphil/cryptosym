# -*- coding: utf-8 -*-

import os
import sys
import yaml
import torch
import random
import argparse
import datetime
import numpy as np
from pathlib import Path

from deep_learning.dataset import HashReversalDataset
from deep_learning.factor import Factor
from deep_learning.supervised_learning import SupervisedLearning


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


def main(dataset_dir):
    random.seed(1)
    np.random.seed(1)
    torch.manual_seed(1)

    config_file = os.path.join(dataset_dir, 'params.yaml')
    factor_file = os.path.join(dataset_dir, 'factors.txt')

    config = load_config(config_file)
    factors = load_factors(factor_file)

    dsets = {
        'train': HashReversalDataset('train', dataset_dir),
        'val': HashReversalDataset('val', dataset_dir),
        'test': HashReversalDataset('test', dataset_dir)
    }

    now = datetime.datetime.now()
    out_dir = 'learning_{:%Y-%m-%d-%H-%M-%S}'.format(now)
    Path(out_dir).mkdir(parents=True, exist_ok=True)

    learning = SupervisedLearning(out_dir, config, factors, dsets)
    with learning:
        try:
            print('Training...')
            learning.train()
        finally:
            print('Saving best model...')
            learning.save_best_model()
            print('Testing...')
            learning.test()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Hash reversal via deep learning')
    parser.add_argument('dataset_dir', type=str,
        help='Path to the dataset directory')
    args = parser.parse_args()
    main(args.dataset_dir)
    print('Done.')
    sys.exit(0)
