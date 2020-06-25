# -*- coding: utf-8 -*-
import os
import numpy as np
from matplotlib import pyplot as plt
from BitVector import BitVector

from graphs.undirected_graph import UndirectedGraph
from graphs.factor_graph import FactorGraph
from utils.probability import Probability
from utils.config import Config
from utils import constants
from utils.log import initLogging, getLogger


def loadDataset(config):
  train, test = [], []
  parts = config.dataset.split('/')[-1].split('.')[0].split('-')
  num_rv = int(parts[1])
  N = int(parts[2])
  N_train = int(N * 0.8)

  data = BitVector(filename=config.dataset)
  for rv in range(num_rv):
    bv = data.read_bits_from_file(N)
    train.append(bv[:N_train])
    test.append(bv[N_train:])
  
  return train, test, (num_rv, N, N_train)


if __name__ == '__main__':
  config = Config()

  initLogging(config.log_dir)
  config.logConfig()

  logger = getLogger('main')

  logger.info('Loading dataset...')
  X_train, X_test, stats = loadDataset(config)
  n = stats[0]  # number of random variables
  N = stats[1]  # number of samples
  N_train = stats[2]  # number of samples used for training
  N_test = N - N_train  # number of samples used for testing
  logger.info('Loaded dataset.')
  
  HASH_INPUT_NBITS = 64 # TODO - this should be stored in a config file with the dataset

  logger.info('train={},\ttest={},\tnum_hash_bits={},\tnum_hash_input_bits={},\tinternal_bits={}'.format(
    N_train, N_test, 256, HASH_INPUT_NBITS, n - 256 - HASH_INPUT_NBITS))

  prob = Probability(X_train)
  udg = UndirectedGraph(config.graph)

  if config.visualize:
    udg.visualizeGraph(os.path.join(config.experiment_dir, 'graph_undirected.png'))

  # Need to build factor graph from the undirected graph
  fg = FactorGraph(prob, udg, config=config)
  if config.visualize:
    fg.visualizeGraph(os.path.join(config.experiment_dir, 'graph_factor.png'))

  """
  TODO -
  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?
    -> Stuff happens in groups of 32 bits, so it makes sense to use 32 connections per node
  
  Technically the test data is used in "training" in MATLAB
  """

  logger.info('Checking accuracy of single bit prediction on test data...')
  correct_count, total_count = 0, 0
  log_likelihood_ratios, accuracies = [], []

  try:
    for i in range(N_test):
      hash_bits = [X_test[hash_bit][i] for hash_bit in range(256)]
      true_hash_input_bit = X_test[constants.BIT_PRED][i]

      observed = dict()
      for rv, hash_val in enumerate(hash_bits):
        observed[rv] = hash_val

      prob_hash_input_bit_is_one, llr = fg.predict(constants.BIT_PRED, 1, observed=observed)

      guess = 1 if prob_hash_input_bit_is_one >= 0.5 else 0
      is_correct = int(guess == true_hash_input_bit)
      correct_count += is_correct
      total_count += 1
      logger.info('\tGuessed {}, true value is {}'.format(guess, true_hash_input_bit))

      log_likelihood_ratios.append(llr)
      accuracies.append(is_correct)

      logger.info('\tAccuracy: {0}/{1} ({2:.3f}%)'.format(
        correct_count, total_count, 100.0 * correct_count / total_count))

  finally:
    if config.visualize:
      log_likelihood_ratios = np.array(log_likelihood_ratios)
      accuracies = np.array(accuracies)
      plt.close()
      fig, axs = plt.subplots(1, 2, sharey=True, tight_layout=True)
      axs[0].set_title('Correct predictions')
      axs[0].set_xlabel('Log-likelihood ratio')
      axs[0].hist(log_likelihood_ratios[accuracies == 1], bins=30)
      axs[1].set_title('Incorrect predictions')
      axs[1].set_xlabel('Log-likelihood ratio')
      axs[1].hist(log_likelihood_ratios[accuracies == 0], bins=30)
      plt.savefig(os.path.join(config.experiment_dir, 'accuracy_distribution.png'))

  logger.info('Done.')
