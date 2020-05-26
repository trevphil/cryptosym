# -*- coding: utf-8 -*-
import numpy as np
from numpy import loadtxt
from matplotlib import pyplot as plt

from graphs.undirected_graph import UndirectedGraph
from graphs.directed_graph import DirectedGraph
from graphs.factor_graph import FactorGraph
from utils.probability import Probability
from generate_dataset import INDEX_OF_BIT_TO_PREDICT as BIT_PRED

VISUALIZE = True
VERBOSE = True
ENTRY_POINT = 0

if __name__ == '__main__':
  dataset = loadtxt('./data/data.csv', delimiter=',')

  train_cutoff = int(dataset.shape[0] * 0.8)
  N = train_cutoff # number of samples
  n = dataset.shape[1] # number of variables

  if VERBOSE:
    print('N={},\tnum_hash_bits={},\tnum_hash_input_bits={}'.format(
        N, 256, n - 256))

  X = dataset
  X_train = X[:N]
  X_test = X[N:]

  if ENTRY_POINT == 0:
    prob = Probability(X_train, verbose=VERBOSE)
    prob.save('./data/prob.npy')
  else:
    prob = Probability(X_train, data='./data/prob.npy', verbose=VERBOSE)

  if ENTRY_POINT <= 1:
    udg = UndirectedGraph(prob, size=(N, n), max_connections=5, verbose=VERBOSE)
    udg.saveGraph('./data/bn_undirected.yaml')
    if VISUALIZE:
      udg.visualizeGraph()

  if ENTRY_POINT <= 2:
    dg = DirectedGraph('./data/bn_undirected.yaml', verbose=VERBOSE)
    dg.saveGraph('./data/bn_directed.yaml')
    if VISUALIZE:
      dg.visualizeGraph()

  fg = FactorGraph(prob, './data/bn_directed.yaml', verbose=VERBOSE)
  if VISUALIZE:
    fg.visualizeGraph()

  """
  TODO -

  LBP diverges if max_connections in undirected graph is large...
  *** Normalize all messages? ***

  CPD is most interesting when # dependencies is between 1 and 15 (limit it?)
    -> because default probability is 0.5 when there is no instance of
       all RVs in a CPD being some assigned value, and this happens more
       when there are more RVs in a CPD --> fix with larger data set

  Think about what would be a natural BN structure...
    -> Should all hash bits in a byte be fully connected?

  What about adding data points generated from "inside" the SHA256 algorithm?
  """

  print('Checking accuracy of single bit prediction on test data...')
  correct_count = 0
  total_count = 0
  probabilities = []
  accuracies = []

  try:
    for i in range(X_test.shape[0]):
      hash = X_test[i, :256]
      true_hash_input_bit = X_test[i, BIT_PRED]

      observed = dict()
      for rv, hash_val in enumerate(hash):
        observed[rv] = hash_val

      converged, _ = fg.loopyBP(observed=observed)
      if not converged: continue;

      success, prob_hash_input_bit_is_one = fg.predict(BIT_PRED, 1)
      if not success: continue;

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
    if VISUALIZE:
      probabilities = np.array(probabilities)
      accuracies = np.array(accuracies)
      fig, axs = plt.subplots(1, 2, sharey=True, tight_layout=True)
      axs[0].set_title('Correct predictions')
      axs[0].set_xlabel('Prob. hash input bit is 1')
      axs[0].hist(probabilities[accuracies == 1], bins=30)
      axs[1].set_title('Incorrect predictions')
      axs[1].set_xlabel('Prob. hash input bit is 1')
      axs[1].hist(probabilities[accuracies == 0], bins=30)
      plt.show()

  print('Done.')
