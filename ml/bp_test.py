# -*- coding: utf-8 -*-
import numpy as np
from itertools import product


class RandomVariable(object):
  def __init__(self, key):
    self.key = key
    self.message_cache = dict()
    self.neighboring_factors = []
    self.default_initialization = 1.0

  def updateNeighboringFactors(self, neighboring_factors):
    self.neighboring_factors = neighboring_factors

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
  def __init__(self, rvs, table):
    self.rvs = rvs
    self.key = ','.join(sorted({str(rv.key) for rv in self.rvs}))
    self.table = table
    self.message_cache = dict()
    self.default_initialization = 1.0
    assert len(self.rvs) == self.table.shape[1] - 1

  def previousMessage(self, to_rv, rv_value):
    tup = (to_rv.key, rv_value)
    return self.message_cache.get(tup, self.default_initialization)

  def message(self, to_rv, rv_value):
    rv_idx = self.rvs.index(to_rv)
    filtered = self.table[self.table[:, rv_idx] == rv_value]
    assert filtered.shape[1] == self.table.shape[1]
    result = 0.0
    for row_idx in range(filtered.shape[0]):
      message_product = 1.0
      for col_idx in range(filtered.shape[1] - 1):
        if col_idx == rv_idx:
          continue
        rv_val = filtered[row_idx, col_idx]
        rv = self.rvs[col_idx]
        message_product *= rv.previousMessage(self, rv_val)
      message_product *= filtered[row_idx, -1]
      result += message_product

    self.message_cache[(to_rv.key, rv_value)] = result
    return result


class FactorGraph(object):
  def __init__(self, random_variables, factors):
    sorter = lambda factor: len(factor.rvs)
    self.factors = list(sorted(factors, key=sorter))
    self.rvs = random_variables
    rv_str = ', '.join([rv.key for rv in self.rvs])
    factor_str = ', '.join(['(' + f.key + ')' for f in self.factors])
    print('FactorGraph initialized...\n\tRVs = %s\n\tfactors = %s' % \
      (rv_str, factor_str))
    """TODO - Enforce ordering on the RVs (is this actually needed?)"""
    """Choose a root, send from leaves to root & vice versa"""


  def freeMemory(self):
    """Get rid of retain cycles between factors and RVs"""
    for rv in self.rvs:
      rv.updateNeighboringFactors([])
    for factor in self.factors:
      factor.rvs, factor.table = None, None
    self.rvs, self.factors = None, None


  def beliefPropagation(self, err=1e-2, max_iterations=100):
    converged = False
    iter = 0
    while not converged and iter < max_iterations:
      converged = True

      # Belif propagation: random variables --> factors
      for factor in self.factors:
        for rv in factor.rvs:
          prev0 = rv.previousMessage(factor, 0)
          prev1 = rv.previousMessage(factor, 1)
          new0 = rv.message(factor, 0)
          new1 = rv.message(factor, 1)
          err0 = abs(prev0 - new0)
          err1 = abs(prev1 - new1)
          converged = converged and err0 < err and err1 < err

      # Belief propagation: factors --> random variables
      for factor in self.factors:
        for rv in factor.rvs:
          prev0 = factor.previousMessage(rv, 0)
          prev1 = factor.previousMessage(rv, 1)
          new0 = factor.message(rv, 0)
          new1 = factor.message(rv, 1)
          err0 = abs(prev0 - new0)
          err1 = abs(prev1 - new1)
          converged = converged and err0 < err and err1 < err
      iter += 1

    if iter >= max_iterations:
      print('Max iterations (%d) reached!' % max_iterations)
    return converged


if __name__ == '__main__':
  table1 = np.array([
  #  A  B  E  f(.)
    [0, 0, 0, 0.25],
    [0, 0, 1, 0.00],
    [0, 1, 0, 0.75],
    [0, 1, 1, 1.00],
    [1, 0, 0, 0.00],
    [1, 0, 1, 0.75],
    [1, 1, 0, 1.00],
    [1, 1, 1, 0.25]
  ])

  table2 = np.array([
  #  B  C  D  f(.)
    [0, 0, 0, 0.9],
    [0, 0, 1, 0.7],
    [0, 1, 0, 0.1],
    [0, 1, 1, 0.3],
    [1, 0, 0, 1.0],
    [1, 0, 1, 0.5],
    [1, 1, 0, 0.0],
    [1, 1, 1, 0.5]
  ])

  rvA = RandomVariable('A')
  rvB = RandomVariable('B')
  rvC = RandomVariable('C')
  rvD = RandomVariable('D')
  rvE = RandomVariable('E')

  f1 = Factor([rvA, rvB, rvE], table1)
  f2 = Factor([rvB, rvC, rvD], table2)

  rvA.updateNeighboringFactors([f1])
  rvB.updateNeighboringFactors([f1, f2])
  rvC.updateNeighboringFactors([f2])
  rvD.updateNeighboringFactors([f2])
  rvE.updateNeighboringFactors([f1])

  fg = FactorGraph([rvA, rvB, rvC, rvD, rvE], [f1, f2])
  print(fg.beliefPropagation())
  fg.freeMemory()
