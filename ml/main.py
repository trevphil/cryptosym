# -*- coding: utf-8 -*-
import os
import numpy as np
import pandas as pd
from matplotlib import pyplot as plt

from graphs.undirected_graph import UndirectedGraph
from graphs.factor_graph import FactorGraph
from utils.probability import Probability
from utils.config import Config
from utils import constants
from utils.log import initLogging, getLogger


def loadDataset(config):
  dataset_chunks = pd.read_csv(config.dataset, chunksize=500000,
                               sep=',', dtype=bool, header=None)
  dataset = None
  for chunk in dataset_chunks:
    dataset = chunk if dataset is None else pd.concat([dataset, chunk])
  return dataset


if __name__ == '__main__':
  config = Config()

  initLogging(config.log_dir)
  config.logConfig()

  logger = getLogger('main')

  logger.info('Loading dataset...')
  dataset = loadDataset(config)
  logger.info('Loaded dataset.')

  train_cutoff = int(dataset.shape[0] * 0.8)
  N = train_cutoff  # number of samples
  n = dataset.shape[1]  # number of variables

  X = dataset
  X_train = X[:N]
  X_test = X[N:]
  X_test.reset_index(drop=True, inplace=True)

  logger.info('train={},\ttest={},\tnum_hash_bits={},\tnum_hash_input_bits={},\tinternal_bits={}'.format(
    X_train.shape[0], X_test.shape[0], 256, constants.HASH_INPUT_NBITS, n - 256 - constants.HASH_INPUT_NBITS))

  prob = Probability(X_train)

  if constants.ENTRY_POINT == 0:
    # Need to calculate mutual information scores between all RVs and then build graph
    udg = UndirectedGraph(prob, size=(N, n), config=config)
    udg.saveFullyConnectedGraph(config.fcg_data_file)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if config.visualize:
      udg.visualizeGraph(os.path.join(config.experiment_dir, 'graph_undirected.png'))

  elif constants.ENTRY_POINT <= 1:
    # Mutual information scores already calculated --> load them & build undirected graph
    udg = UndirectedGraph(prob, size=(N, n), config=config, fc_graph=constants.FCG_DATA_FILE)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if config.visualize:
      udg.visualizeGraph(os.path.join(config.experiment_dir, 'graph_undirected.png'))

  # Need to build factor graph from the undirected graph
  fg = FactorGraph(prob, constants.UDG_DATA_FILE, config=config)
  if config.visualize:
    fg.visualizeGraph(os.path.join(config.experiment_dir, 'graph_factor.png'))

  """
  TODO -
  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?
    -> Stuff happens in groups of 32 bits, so it makes sense to use 32 connections per node
  """

  logger.info('Checking accuracy of single bit prediction on test data...')
  correct_count, total_count = 0, 0
  log_likelihood_ratios, accuracies = [], []

  try:
    for i in range(X_test.shape[0]):
      hash_bits = X_test.iloc[i, :256]
      true_hash_input_bit = int(X_test.iat[i, constants.BIT_PRED])

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
