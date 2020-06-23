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


if __name__ == '__main__':
  config = Config()

  constants.makeDataDirectoryIfNeeded()
  constants.makeExperimentDirectoryIfNeeded()
  constants.makeLogDirectoryIfNeeded()

  initLogging(constants.LOG_DIR)
  logger = getLogger('main')

  dataset_chunks = pd.read_csv(constants.DATASET_FILE, chunksize=500000,
                               sep=',', dtype=bool, header=None)
  dataset = pd.concat([chunk for chunk in dataset_chunks])

  train_cutoff = int(dataset.shape[0] * 0.8)
  N = train_cutoff  # number of samples
  n = dataset.shape[1]  # number of variables

  logger.info('train={},\ttest={},\tnum_hash_bits={},\tnum_hash_input_bits={},\tinternal_bits={}'.format(
    N, dataset.shape[0] - N, 256, constants.HASH_INPUT_NBITS, n - 256 - constants.HASH_INPUT_NBITS))

  X = dataset
  X_train = X[:N]
  X_test = X[N:]

  if constants.ENTRY_POINT == 0:
    # Probabilities need to be calculated
    prob = Probability(X_train)
    prob.save(constants.PROB_DATA_FILE)
  else:
    # Probabilities are already saved --> load them
    prob = Probability(X_train, data=constants.PROB_DATA_FILE)

  if constants.ENTRY_POINT <= 1:
    # Need to calculate mutual information scores between all RVs and then build graph
    udg = UndirectedGraph(prob, size=(N, n), config=config)
    udg.saveFullyConnectedGraph(constants.FCG_DATA_FILE)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if config.visualize:
      udg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_undirected.png'))

  elif constants.ENTRY_POINT <= 2:
    # Mutual information scores already calculated --> load them & build undirected graph
    udg = UndirectedGraph(prob, size=(N, n), config=config, fc_graph=constants.FCG_DATA_FILE)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if config.visualize:
      udg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_undirected.png'))

  # Need to build factor graph from the undirected graph
  fg = FactorGraph(prob, constants.UDG_DATA_FILE)
  if config.visualize:
    fg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_factor.png'))

  """
  TODO -
  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?
  """

  logger.info('Checking accuracy of single bit prediction on test data...')
  correct_count, total_count = 0, 0
  log_likelihood_ratios, accuracies = [], []

  try:
    for i in range(X_test.shape[0]):
      hash_bits = X_test[i, :256]
      true_hash_input_bit = X_test[i, constants.BIT_PRED]

      observed = dict()
      for rv, hash_val in enumerate(hash_bits):
        observed[rv] = hash_val

      prob_hash_input_bit_is_one, llr = fg.predict(constants.BIT_PRED, 1,
        observed=observed, visualize_convergence=config.visualize)

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
      plt.savefig(os.path.join(constants.EXPERIMENT_DIR, 'accuracy_distribution.png'))

  logger.info('Done.')
