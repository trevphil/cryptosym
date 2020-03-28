# -*- coding: utf-8 -*-
import numpy as np
from numpy import loadtxt
from graphs.undirected_graph import UndirectedGraph
from graphs.directed_graph import DirectedGraph
from graphs.factor_graph import FactorGraph

from utils.probability import Probability

VISUALIZE = True
VERBOSE = True

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

  prob = Probability(X_train)

  udg = UndirectedGraph(prob, size=(N, n), max_connections=4, verbose=VERBOSE)
  udg.saveGraph('./data/bn_undirected.yaml')
  if VISUALIZE:
    udg.visualizeGraph()

  dg = DirectedGraph('./data/bn_undirected.yaml', verbose=VERBOSE)
  dg.saveGraph('./data/bn_directed.yaml')
  if VISUALIZE:
    dg.visualizeGraph()

  fg = FactorGraph(prob, './data/bn_directed.yaml', verbose=VERBOSE)
  if VISUALIZE:
    fg.visualizeGraph()

  correct_count = 0
  total_count = 0

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
  for i in range(X_test.shape[0]):
    hash = X_test[i, :256]
    true_hash_input_bit = X_test[i, 256]

    observed = dict()
    for rv, hash_val in enumerate(hash):
      observed[rv] = hash_val

    fg.loopyBP(observed=observed)
    prob_hash_input_bit_is_one = fg.predict(256, 1)
    assert not np.isnan(prob_hash_input_bit_is_one), 'probability is NaN!'

    print(prob_hash_input_bit_is_one)

    guess = 0
    if prob_hash_input_bit_is_one >= 0.5:
      guess = 1
    correct_count += (guess == true_hash_input_bit)
    total_count += 1
    print('\tAccuracy: {0:.3f}%'.format(100.0 * correct_count / total_count))

  print('Done.')
