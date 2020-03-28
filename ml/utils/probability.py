# -*- coding: utf-8 -*-
import numpy as np
import networkx as nx
from math import log
from itertools import product
from operator import mul
from functools import reduce

######################################################
#################### PROBABILITY #####################
######################################################

class Probability(object):

  def __init__(self, samples):
    self.samples = samples
    self.N = samples.shape[0] # number of samples
    self.n = samples.shape[1] # number of variables
    self.phats = np.array(
      [[self.pHat([(i, 0)]), self.pHat([(i, 1)])] for i in range(self.n)])


  def count(self, random_variables):
    assert len(random_variables) > 0

    rv_index, rv_value = random_variables[0]
    search = self.samples[:, rv_index] == rv_value

    for rv_index, rv_value in random_variables[1:]:
      search = search & (self.samples[:, rv_index] == rv_value)

    return self.samples[search].shape[0]


  def pHat(self, random_variables):
    return float(self.count(random_variables)) / self.N


  def iHat(self, rv_indices):
    """Mutual information score"""
    bit_sequences = product([0, 1], repeat=len(rv_indices))
    total = 0

    for rv_values in bit_sequences:
      rv_assignments = list(zip(rv_indices, rv_values))
      phat = self.pHat(rv_assignments)
      denominator = 1.0
      for rv_index, rv_val in rv_assignments:
        denominator *= self.phats[rv_index, rv_val]
      total += phat * log(phat / denominator)

    return total

######################################################
######## CONDITIONAL PROBABILITY DISTRIBUTION ########
######################################################

class CPD(object):
  """Conditinal probability distribution"""

  def __init__(self, rv, dependencies):
    all_vars = [rv] + dependencies
    assert len(all_vars) == len(set(all_vars))
    self.rv = rv
    self.dependencies = dependencies

  def allVars(self):
    return [self.rv] + self.dependencies

  def probability_rv_one(self, dependency_values, prob_util):
    rv_assignments = []
    for idx, rv in enumerate(self.dependencies):
      rv_assignments.append((rv, dependency_values[idx]))

    query1 = rv_assignments + [(self.rv, 1)]
    query2 = rv_assignments + [(self.rv, 0)]

    numerator = prob_util.count(query1)
    denominator = numerator + prob_util.count(query2)

    # TODO - Non-optimal solution, need to evaluate how important this is...
    return (0.01 + float(numerator)) / (0.02 + float(denominator))
