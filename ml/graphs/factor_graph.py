# -*- coding: utf-8 -*-
import os
import math
import networkx as nx
import matplotlib.pyplot as plt
import numpy as np
from itertools import product
from math import tanh, atanh, log

from utils.probability import CPD
from utils.constants import LBP_MAX_ITER, EXPERIMENT_DIR

######################################################
################# FACTOR GRAPH NODE ##################
######################################################

class FactorGraphNode(object):
  def __init__(self):
    self.message_cache = dict()
    self.default_initialization = 0.0


  def setInitialization(self, new_init):
    self.default_initialization = new_init


  def reset(self):
    self.message_cache = dict()


  def prevMessage(self, to_node):
    return self.message_cache.get(to_node.key, self.default_initialization)

######################################################
################## RANDOM VARIABLE ###################
######################################################

class RandomVariable(FactorGraphNode):
  def __init__(self, key):
    super().__init__()
    self.key = key
    self.neighboring_factors = []


  def addNeighboringFactor(self, neighbor):
    self.neighboring_factors.append(neighbor)


  def update(self):
    """ Update the messages from this random variable to neighboring factors """
    for factor in self.neighboring_factors:
      total = sum(f.prevMessage(self) for f in self.neighboring_factors if f is not factor)
      self.message_cache[factor.key] = self.default_initialization + total


  def probIsOne(self):
    """ Probability that this random variable is 1, using messages of neighbor factors """
    x = self.default_initialization
    x += sum(f.prevMessage(self) for f in self.neighboring_factors)
    # An alternative would be to check if x < 0, but it doesn't give probability in [0, 1]
    return float(1.0 / (1.0 + pow(math.e, x)))

######################################################
####################### FACTOR #######################
######################################################

class Factor(FactorGraphNode):
  def __init__(self, cpd, rvs, prob_util):
    super().__init__()
    self.query = [rv for rv in rvs if rv.key == cpd.rv][0]
    self.rvs = [rv for rv in rvs if rv.key in cpd.allVars()]
    self.key = ','.join(sorted(str(rv.key) for rv in self.rvs))

    nvars = len(self.rvs)
    nrows = int(2 ** nvars)
    self.table = np.zeros((nrows, nvars + 1), dtype=np.longdouble)

    i = 0
    bit_sequences = product([0, 1], repeat=len(cpd.dependencies))

    for bit_sequence in bit_sequences:
      prob_rv_one = cpd.probability_rv_one(
        dependency_values=bit_sequence, prob_util=prob_util)

      self.table[i, :-1]  = (0,) + bit_sequence
      self.table[i, -1]   = 1.0 - prob_rv_one
      i += 1

      self.table[i, :-1]  = (1,) + bit_sequence
      self.table[i, -1]   = prob_rv_one
      i += 1

    assert len(self.rvs) == self.table.shape[1] - 1
  

  def update(self):
    """ Update the messages from this factor to neighboring random variables """
    for rv in self.rvs:
      prod = 1.0

      for other_rv in self.rvs:
        if rv is not other_rv:
          prod *= tanh(rv.prevMessage(self) / 2.0)

      if prod != 1.0:
        # TODO - Not sure if the algo still works with this condition.
        #        Factors with only 1 RV will never be updated.
        self.message_cache[rv.key] = 2.0 * atanh(prod)

######################################################
################### FACTOR GRAPH #####################
######################################################

class FactorGraph(object):

  def __init__(self, prob_util, directed_graph_yaml, verbose=True):
    self.prob_util = prob_util
    self.verbose = verbose
    self.num_predictions = 0
    self.graph = nx.Graph()
    self.rvs = []

    directed_graph = nx.read_yaml(directed_graph_yaml)
    max_fac = 0
    cpds = []

    for rv in directed_graph.nodes():
      self.graph.add_node(rv, bipartite=0)
      dependencies = [e[0] for e in directed_graph.in_edges(rv)]
      cpd = CPD(rv, dependencies)
      cpds.append(cpd)
      max_fac = max(max_fac, len(cpd.allVars()) - 1)
      self.rvs.append(RandomVariable(rv))

    if self.verbose:
      print('Creating %d factors (max factor size is %d)...' % \
        (len(cpds), max_fac))

    self.factors = [Factor(cpd, self.rvs, self.prob_util) for cpd in cpds]
    sorter = lambda factor: len(factor.rvs)
    self.factors = list(sorted(self.factors, key=sorter))

    for factor in self.factors:
      self.graph.add_node(factor.key, bipartite=1)
      for rv in factor.rvs:
        self.graph.add_edge(factor.key, rv.key)
        rv.addNeighboringFactor(factor)
    
    if self.verbose:
      print('Creating bipartite adjacency matrix H...')
    M, N = len(self.factors), len(self.rvs)
    self.H = np.zeros((M, N), dtype='int')
    for factor_idx in range(M):
      for rv_idx in range(N):
        factor, rv = self.factors[factor_idx], self.rvs[rv_idx]
        self.H[factor_idx, rv_idx] = (rv in factor.rvs)

    if self.verbose:
      print('All %d factors initialized.' % len(self.factors))


  def visualizeGraph(self, img_file):
    if self.verbose:
      print('Visualizing factor graph...')

    plt.close()
    first_partition = [f.key for f in self.factors]
    pos = nx.drawing.layout.bipartite_layout(self.graph, first_partition)
    nx.draw_networkx(self.graph, pos=pos, with_labels=False,
      width=0.1, node_size=5)
    plt.savefig(img_file)


  def setup(self, observed):
    """
    `observed` is a dict mapping from RV key --> RV value and we need
    to re-map the RV key (node #) to a column index in `factor.table`,
    which can be done by finding the index of the RV in `factor.rvs` whose
    key (node #) matches the RV key in the `observed` dict.
    ---
    This LBP algorithm is using a logarithmic version of the sum-product
    algorithm for increased numerical stability. Messages for factor nodes
    in the factor graph should be initialized to 0.0 and messages for
    random variable nodes should be initialized with the log-likelihood
    ratio (LLR) of the RV, i.e. log[P(x=0|observed) / P(x=1|observed)].
    ---
    See:
    'Efficient Implementations of the Sum-Product Algorithm for Decoding LDPC Codes'
    """

    for rv in self.rvs:
      rv.reset()
    for factor in self.factors:
      factor.reset()
    
    for rv in self.rvs:
      factor = [f for f in self.factors if f.query is rv][0]
      search = factor.table[:, 0] == 1 # Select all rows where column 0 is 1 (rv=1)
      for observed_rv_key, observed_rv_value in observed.items():
        match = [(i, rv2) for i, rv2 in enumerate(factor.rvs) if rv2.key == observed_rv_key]
        if len(match) > 0:
          idx = match[0][0]
          search = search & (factor.table[:, idx] == observed_rv_value)
      filtered = factor.table[search]
      assert filtered.shape[1] == factor.table.shape[1]
      
      prob_rv_one = np.sum(filtered[:, -1]) # Marginalize out the unobserved variables
      rv.setInitialization(log(1.0 - prob_rv_one) / log(prob_rv_one))


  def loopyBP(self, intermediate_pred=None, max_iterations=LBP_MAX_ITER):
    """
    https://arxiv.org/pdf/1301.6725.pdf
    https://www.ski.org/sites/default/files/publications/bptutorial.pdf
    https://www.mit.edu/~6.454/www_fall_2002/lizhong/factorgraph.pdf
    https://en.wikipedia.org/wiki/Junction_tree_algorithm - TODO try it out, maybe
    Noisy-OR: https://people.csail.mit.edu/dsontag/papers/HalpernSontag_uai13.pdf
              https://www.sciencedirect.com/science/article/pii/B9781483214511500160?via%3Dihub
    ---
    `observed` is a dictionary mapping RV index --> RV value, for example,
    mapping all 0-255 hash bits to their observed values.
    """

    if self.verbose:
      print('Running loopy belief propagation...')

    itr, predictions = 0, []
    
    while itr < max_iterations:
      try:
        prediction = intermediate_pred()
        predictions.append(prediction)
      except Exception as e:
        print('Error during intermediate prediction: {}'.format(e))
        exit()
      
      for factor in self.factors:
        factor.update()
      for rv in self.rvs:
        rv.update()

      if self.isConverged():
        break
      itr += 1

    if self.verbose:
      if itr >= max_iterations:
        print('Loopy BP did not converge, max iterations reached')
      else:
        print('Loopy BP converged in %d iterations.' % (itr + 1))
    return predictions
  
  
  def isConverged(self):
    """ Converged if u * transpose(H) is all zeros """
    u = np.zeros(len(self.rvs), dtype='int')
    for idx, rv in enumerate(self.rvs):
      u[idx] = 1 if rv.probIsOne() > 0.5 else 0
    all_zeros = not u.dot(self.H.T).any()
    return all_zeros


  def predict(self, rv_index, rv_value, observed=dict(), visualize_convergence=False):
    """
    Predict probability that the RV has the given value.
    This will first run loopy belief propagation, with observed variables if desired.
    ---
    `observed` is a dictionary mapping RV index --> RV value, for example,
    mapping all 0-255 hash bits to their observed values.
    """

    def _predict():
      """ Helper function, also called during LBP to see how prediction changes """
      rv = [rv for rv in self.rvs if rv.key == rv_index][0]
      p = rv.probIsOne()
      return p if rv_value == 1 else (1.0 - p)

    self.setup(observed)
    preds = self.loopyBP(intermediate_pred=_predict)

    if visualize_convergence:
      plt.clf()
      plt.plot(preds)
      plt.xlabel('Iteration')
      plt.ylabel('Probability')
      img_file = os.path.join(EXPERIMENT_DIR, 'lbp_%04d.png' % self.num_predictions)
      plt.savefig(img_file)

    self.num_predictions += 1
    return _predict()
