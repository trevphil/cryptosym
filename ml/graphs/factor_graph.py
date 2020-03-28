# -*- coding: utf-8 -*-
import networkx as nx
import matplotlib.pyplot as plt
import numpy as np
from itertools import product

from utils.probability import CPD


class RandomVariable(object):
  def __init__(self, key):
    self.key = key
    self.message_cache = dict()
    self.neighboring_factors = []
    self.default_initialization = 1.0

  def addNeighboringFactor(self, neighbor):
    self.neighboring_factors.append(neighbor)

  def reset(self):
    self.message_cache = dict()

  def previousMessage(self, to_factor, rv_value):
    tup = (to_factor.key, rv_value)
    return self.message_cache.get(tup, self.default_initialization)

  def message(self, to_factor, rv_value):
    result = 1.0
    for factor in self.neighboring_factors:
      if factor is to_factor:
        continue
      result *= factor.previousMessage(self, rv_value)

    self.message_cache[(to_factor.key, rv_value)] = result
    return result


class Factor(object):
  def __init__(self, cpd, rvs, prob_util):
    self.rvs = [rv for rv in rvs if rv.key in cpd.allVars()]
    self.key = ','.join(sorted({str(rv.key) for rv in self.rvs}))
    self.message_cache = dict()
    self.default_initialization = 1.0

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

  def reset(self):
    self.message_cache = dict()

  def previousMessage(self, to_rv, rv_value):
    tup = (to_rv.key, rv_value)
    return self.message_cache.get(tup, self.default_initialization)

  def message(self, to_rv, rv_value, observed):
    """
    `observed` is a dict mapping from RV key --> RV value and we need
    to re-map the RV key (node #) to a column index in `self.table`,
    which can be done by finding the index of the RV in `self.rvs` whose
    key (node #) matches the RV key in the `observed` dict.
    """
    rv_idx = self.rvs.index(to_rv)
    search = self.table[:, rv_idx] == rv_value
    for observed_rv, observed_rv_value in observed.items():
      match = [(i, rv) for i, rv in enumerate(self.rvs) if rv.key == observed_rv]
      if len(match) > 0:
        idx = match[0][0]
        search = search & (self.table[:, idx] == observed_rv_value)

    filtered = self.table[search]
    assert filtered.shape[1] == self.table.shape[1]

    result = 0.0
    for row_idx in range(filtered.shape[0]):
      message_product = 1.0
      for col_idx in range(filtered.shape[1] - 1):
        if col_idx == rv_idx: continue;
        rv_val = filtered[row_idx, col_idx]
        rv = self.rvs[col_idx]
        message_product *= rv.previousMessage(self, rv_val)
      message_product *= filtered[row_idx, -1]
      result += message_product

    self.message_cache[(to_rv.key, rv_value)] = result
    return result


class FactorGraph(object):

  def __init__(self, prob_util, directed_graph_yaml, verbose=True):
    self.prob_util = prob_util
    self.verbose = verbose

    self.graph = nx.Graph()
    directed_graph = nx.read_yaml(directed_graph_yaml)
    max_fac = 0
    cpds = []
    self.rvs = []

    for rv in directed_graph.nodes():
      self.graph.add_node(rv, bipartite=0)
      dependencies = [e[0] for e in directed_graph.in_edges(rv)]
      cpd = CPD(rv, dependencies)
      cpds.append(cpd)
      max_fac = max(max_fac, len(cpd.allVars()))
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
      print('All %d factors initialized.' % len(self.factors))


  def visualizeGraph(self):
    if self.verbose:
      print('Showing factor graph...')

    first_partition = [f.key for f in self.factors]
    pos = nx.drawing.layout.bipartite_layout(self.graph, first_partition)
    nx.draw_networkx(self.graph, pos=pos, with_labels=False,
      width=0.1, node_size=5)
    plt.show()


  def reset(self):
    for rv in self.rvs:
      rv.reset()
    for factor in self.factors:
      factor.reset()


  def loopyBP(self, observed=dict(), err_tol=0.01, max_iterations=20):
    """
    `observed` is a dictionary mapping RV index --> RV value, for example,
    mapping all 0-255 hash bits to their observed values.
    """

    if self.verbose:
      print('Running loopy belief propagation...')

    self.reset()
    converged = False
    iter = 0

    while not converged and iter < max_iterations:
      converged = True

      for factor in self.factors:
        for rv in factor.rvs:
          # Belif propagation: random variables --> factors
          prev0 = rv.previousMessage(factor, 0)
          prev1 = rv.previousMessage(factor, 1)
          new0 = rv.message(factor, 0)
          new1 = rv.message(factor, 1)
          err0, err1 = abs(prev0 - new0), abs(prev1 - new1)
          assert not np.isnan(err0), 'RV->factor : err0 = NaN'
          assert not np.isnan(err1), 'RV->factor : err1 = NaN'
          converged = converged and err0 < err_tol and err1 < err_tol

          # Belief propagation: factors --> random variables
          prev0 = factor.previousMessage(rv, 0)
          prev1 = factor.previousMessage(rv, 1)
          new0 = factor.message(rv, 0, observed)
          new1 = factor.message(rv, 1, observed)
          err0, err1 = abs(prev0 - new0), abs(prev1 - new1)
          assert not np.isnan(err0), 'factor->RV : err0 = NaN'
          assert not np.isnan(err1), 'factor->RV : err1 = NaN'
          converged = converged and err0 < err_tol and err1 < err_tol
      iter += 1

    if self.verbose:
      if converged:
        print('Loopy BP converged in %d iterations.' % iter)
      else:
        print('Loopy BP did not converge, max iterations reached')

    return converged, iter


  def predict(self, rv_index, rv_value):
    """
    Predict probability that the RV has the given value.
    Assume LBP has already been run, with observed variables if desired.
    """

    rv = [rv for rv in self.rvs if rv.key == rv_index][0]
    factors = [f for f in self.factors if rv in f.rvs]

    total = 1.0
    total_opposite = 1.0
    for factor in factors:
      total *= factor.previousMessage(rv, rv_value)
      total_opposite *= factor.previousMessage(rv, 1 - rv_value)

    if total + total_opposite == 0:
      print('WARNING: prediction would divide by zero!')
      return 0.5
    return total / (total + total_opposite)
