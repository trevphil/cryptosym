# -*- coding: utf-8 -*-
import os
import math
import networkx as nx
import matplotlib.pyplot as plt
import numpy as np
from itertools import product
from math import tanh, atanh, log

from utils.log import getLogger
from utils.constants import LBP_MAX_ITER, EPSILON

######################################################
################# FACTOR GRAPH NODE ##################
######################################################

class FactorGraphNode(object):
  def __init__(self):
    self.message_cache = dict()
    self.default_initialization = 0.0
    self.key = None


  def setInitialization(self, new_init):
    self.default_initialization = new_init


  def reset(self):
    self.message_cache = dict()


  def message(self, to_node):
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
      total = sum(f.message(self) for f in self.neighboring_factors if f is not factor)
      self.message_cache[factor.key] = self.default_initialization + total


  def isOne(self):
    """
    Returns a tuple where the first element is `True` if the random variable
    is predicted to be 1 (based on the messages of neighboring factors) and
    `False` otherwise. The second element of the tuple is the log-likelihood ratio,
    log[P(x=0) / P(x=1)]. The LLR is < 0 if the variable is predicted to be 1.
    """
    x = self.default_initialization
    x += sum(f.message(self) for f in self.neighboring_factors)
    return (x < 0), x

######################################################
####################### FACTOR #######################
######################################################

class Factor(FactorGraphNode):
  def __init__(self, rvs):
    super().__init__()
    self.rvs = rvs
    self.key = ','.join(sorted(str(rv.key) for rv in self.rvs))


  def update(self):
    """ Update the messages from this factor to neighboring random variables """
    for rv in self.rvs:
      prod = 1.0

      for other_rv in self.rvs:
        if rv is not other_rv:
          prod *= tanh(other_rv.message(self) / 2.0)

      if abs(prod) != 1.0:
        # TODO - Not sure if the algo still works with this condition.
        #        Factors with only 1 RV will never be updated.
        self.message_cache[rv.key] = 2.0 * atanh(prod)

######################################################
################### FACTOR GRAPH #####################
######################################################

class FactorGraph(object):

  def __init__(self, prob_util, udg, config):
    self.logger = getLogger('factor_graph')
    self.config = config
    self.prob_util = prob_util
    self.num_predictions = 0
    self.bipartite_graph = nx.Graph()
    self.rvs = []
    self.factors = []

    self.undirected_graph = udg.graph

    for rv in self.undirected_graph.nodes():
      self.bipartite_graph.add_node(rv, bipartite=0)
      self.rvs.append(RandomVariable(rv))
    
    factor_keys = set()
    for rv in self.rvs:
      neighbor_rv_keys = self.undirected_graph[rv.key].keys()
      factor_rvs = [v for v in self.rvs if (v is rv or v.key in neighbor_rv_keys)]
      new_factor = Factor(factor_rvs)
      if new_factor.key not in factor_keys:
        self.factors.append(new_factor)
        factor_keys.add(new_factor.key)

    for factor in self.factors:
      self.bipartite_graph.add_node(factor.key, bipartite=1)
      for rv in factor.rvs:
        self.bipartite_graph.add_edge(factor.key, rv.key)
        rv.addNeighboringFactor(factor)

    self.logger.info('All %d factors initialized.' % len(self.factors))


  def visualizeGraph(self, img_file):
    self.logger.info('Visualizing factor graph...')

    plt.close()
    first_partition = [f.key for f in self.factors]
    pos = nx.drawing.layout.bipartite_layout(self.bipartite_graph, first_partition)
    nx.draw_networkx(self.bipartite_graph, pos=pos, with_labels=False,
      width=0.1, node_size=5)
    plt.savefig(img_file)


  def setup(self, observed):
    """
    `observed` is a dict mapping from RV key --> RV value
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
      neighbors = set(self.undirected_graph[rv.key].keys())
      rv_assignments = [(rv_key, val) for rv_key, val in observed.items() if rv_key in neighbors]
      rv_assignments.append((rv.key, 0))
      count0 = self.prob_util.count(rv_assignments)
      rv_assignments[-1] = (rv.key, 1)
      count1 = self.prob_util.count(rv_assignments)
      total = count0 + count1

      if total == 0:
        prob_rv_one = 0.5
      else:
        # EPSILON affects the model such that no event can have 0.0 or 1.0 probability
        prob_rv_one = max(EPSILON, min(1.0 - EPSILON, float(count1) / total))

      rv.setInitialization(log((1.0 - prob_rv_one) / prob_rv_one))


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

    self.logger.info('Running loopy belief propagation...')

    itr, log_likelihood_ratios = 0, []
    
    while itr < max_iterations:
      try:
        _, llr = intermediate_pred()
        log_likelihood_ratios.append(llr)
      except Exception as e:
        self.logger.error('Error during intermediate prediction: {}'.format(e))
        exit()
      
      for factor in self.factors:
        factor.update()
      for rv in self.rvs:
        rv.update()

      if self.isConverged():
        break
      itr += 1

    if itr >= max_iterations:
      self.logger.warn('Loopy BP did not converge, max iterations reached')
    else:
      self.logger.info('Loopy BP converged in %d iterations.' % (itr + 1))
    return log_likelihood_ratios
  
  
  def isConverged(self):
    return False # TODO


  def predict(self, rv_index, rv_value, observed=dict()):
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
      is_one, llr = rv.isOne()
      prediction = int(is_one) if rv_value == 1 else 1 - int(is_one)
      return prediction, llr

    self.setup(observed)
    log_likelihood_ratios = self.loopyBP(intermediate_pred=_predict)

    if self.config.visualize:
      plt.clf()
      plt.plot(log_likelihood_ratios)
      plt.xlabel('Iteration')
      plt.ylabel('Log likelihood ratio')
      img_file = os.path.join(self.config.experiment_dir, 'lbp_%04d.png' % self.num_predictions)
      plt.savefig(img_file)

    self.num_predictions += 1
    return _predict()
