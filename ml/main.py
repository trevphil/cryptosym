# -*- coding: utf-8 -*-
import os
import numpy as np
from numpy import loadtxt
from matplotlib import pyplot as plt

from graphs.undirected_graph import UndirectedGraph
from graphs.directed_graph import DirectedGraph
from graphs.factor_graph import FactorGraph
from utils.probability import Probability
from utils import constants

if __name__ == '__main__':
  constants.makeDataDirectoryIfNeeded()
  constants.makeExperimentDirectoryIfNeeded()

  dataset = loadtxt(constants.DATASET_FILE, delimiter=',')

  train_cutoff = int(dataset.shape[0] * 0.8)
  N = train_cutoff # number of samples
  n = dataset.shape[1] # number of variables

  if constants.VERBOSE:
    print('N={},\tnum_hash_bits={},\tnum_hash_input_bits={}'.format(
        N, 256, n - 256))

  X = dataset
  X_train = X[:N]
  X_test = X[N:]

  if constants.ENTRY_POINT == 0:
    # Probabilities need to be calculated
    prob = Probability(X_train, verbose=constants.VERBOSE)
    prob.save(constants.PROB_DATA_FILE)
  else:
    # Probabilities are already saved --> load them
    prob = Probability(X_train, data=constants.PROB_DATA_FILE, verbose=constants.VERBOSE)
  
  if constants.ENTRY_POINT <= 1:
    # Need to calculate mutual information scores between all RVs and then build graph
    udg = UndirectedGraph(prob, size=(N, n), verbose=constants.VERBOSE)
    udg.saveFullyConnectedGraph(constants.FCG_DATA_FILE)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if constants.VISUALIZE:
      udg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_undirected.png'))

  elif constants.ENTRY_POINT <= 2:
    # Mutual information scores already calculated --> load them & build undirected graph
    udg = UndirectedGraph(prob, size=(N, n), fc_graph=constants.FCG_DATA_FILE,
                          verbose=constants.VERBOSE)
    udg.saveUndirectedGraph(constants.UDG_DATA_FILE)
    if constants.VISUALIZE:
      udg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_undirected.png'))

  if constants.ENTRY_POINT <= 3:
    # Need to make DAG --> assign directions from the undirected graph
    dg = DirectedGraph(constants.UDG_DATA_FILE, verbose=constants.VERBOSE)
    dg.saveGraph(constants.DG_DATA_FILE)
    if constants.VISUALIZE:
      dg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_directed.png'))

  # Need to build factor graph from the directed graph
  fg = FactorGraph(prob, constants.DG_DATA_FILE, verbose=constants.VERBOSE)
  if constants.VISUALIZE:
    fg.visualizeGraph(os.path.join(constants.EXPERIMENT_DIR, 'graph_factor.png'))

  """
  TODO -
  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?

  What about adding data points generated from "inside" the SHA256 algorithm?
  """

  print('Checking accuracy of single bit prediction on test data...')
  correct_count, total_count = 0, 0
  probabilities, accuracies = [], []

  try:
    for i in range(X_test.shape[0]):
      hash_bits = X_test[i, :256]
      true_hash_input_bit = X_test[i, constants.BIT_PRED]

      observed = dict()
      for rv, hash_val in enumerate(hash_bits):
        observed[rv] = hash_val

      success, prob_hash_input_bit_is_one = fg.predict(constants.BIT_PRED, 1,
        observed=observed, visualize_convergence=constants.VISUALIZE)
      if not success:
        continue

      print(prob_hash_input_bit_is_one)

      guess = 1 if prob_hash_input_bit_is_one >= 0.5 else 0
      is_correct = int(guess == true_hash_input_bit)
      correct_count += is_correct
      total_count += 1

      probabilities.append(prob_hash_input_bit_is_one)
      accuracies.append(is_correct)

      print('\tAccuracy: {0}/{1} ({2:.3f}%)'.format(
        correct_count, total_count, 100.0 * correct_count / total_count))

  finally:
    if constants.VISUALIZE:
      probabilities = np.array(probabilities)
      accuracies = np.array(accuracies)
      plt.close()
      fig, axs = plt.subplots(1, 2, sharey=True, tight_layout=True)
      axs[0].set_title('Correct predictions')
      axs[0].set_xlabel('Prob. hash input bit is 1')
      axs[0].hist(probabilities[accuracies == 1], bins=30)
      axs[1].set_title('Incorrect predictions')
      axs[1].set_xlabel('Prob. hash input bit is 1')
      axs[1].hist(probabilities[accuracies == 0], bins=30)
      plt.savefig(os.path.join(constants.EXPERIMENT_DIR, 'accuracy_distribution.png'))

  print('Done.')
