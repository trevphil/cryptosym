# -*- coding: utf-8 -*-
import numpy as np
from BitVector import BitVector

from utils.log import getLogger

######################################################
#################### PROBABILITY #####################
######################################################

class Probability(object):

  def __init__(self, data):
    """
    Parameters
      - data: an array of BitVectors where the i_th BitVector corresponds to
              random variable "i" and has a list of all the values taken by
              that random variable during dataset generation (column = sample index)
    """

    self.data = data
    self.logger = getLogger('probability')
    self.n = len(data)  # number of random variables
    self.N = len(data[0])  # number of samples

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

    result = BitVector(bitstring='1' * self.N)
    for rv_index, rv_value in random_variables:
      samples = self.data[rv_index]
      if rv_value == 0:
        samples = ~samples
      result = result & samples

    return result.count_bits()


  def pHat(self, random_variables):
    """
    Accepts an array of tuples where each tuple is the random variable index
    and the value taken by that random variable. Returns the probability that
    the particular configuration of random variables will occur.
    """
    return float(self.count(random_variables)) / self.N
