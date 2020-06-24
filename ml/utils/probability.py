# -*- coding: utf-8 -*-
import numpy as np
import networkx as nx
from math import log
from itertools import product
from operator import mul
from functools import reduce

from utils.log import getLogger

# The logger is declared at top-level here because otherwise there
# is an error when pickling a `Probability` object in Python multiprocessing
logger = getLogger('probability')

######################################################
#################### PROBABILITY #####################
######################################################

class Probability(object):

  def __init__(self, samples):
    """
    Parameters
      - samples: a pandas dataframe where each row is a sample and each
                 column a random variable index (hash bits + hash input + internals)
    """

    self.samples = samples
    self.N = samples.shape[0]  # number of samples
    self.n = samples.shape[1]  # number of variables

    # phats[rv_idx, rv_val] = probability that the RV has that value (1 or 0)
    self.phats = np.zeros((self.n, 2))
    for rv in range(self.n):
      phat = self.pHat([(rv, 0)])
      self.phats[rv, :] = (phat, 1.0 - phat)


  def count(self, random_variables):
    """
    Accepts an array of tuples where each tuple is the random variable index
    and the value taken by that random variable. Returns the number of samples
    in the dataset where the RVs take their assigned values.
    """
    assert len(random_variables) > 0

    rv_index, rv_value = random_variables[0]
    search = (self.samples.iloc[:, rv_index] == rv_value)

    for rv_index, rv_value in random_variables[1:]:
      search = search & (self.samples.iloc[:, rv_index] == rv_value)

    return self.samples.iloc[search.values].shape[0]


  def pHat(self, random_variables):
    """
    Accepts an array of tuples where each tuple is the random variable index
    and the value taken by that random variable. Returns the probability that
    the particular configuration of random variables will occur.
    """
    return float(self.count(random_variables)) / self.N
