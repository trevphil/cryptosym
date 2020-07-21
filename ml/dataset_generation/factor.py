# -*- coding: utf-8 -*-
import networkx as nx
from enum import Enum, unique

@unique
class FactorType(Enum):
  AND = 'AND'
  XOR = 'XOR'
  XOR_C0 = 'XOR_C0'
  XOR_C1 = 'XOR_C1'
  OR = 'OR'
  INV = 'INV'
  SHIFT = 'SHIFT'

  @staticmethod
  def numInputs(factor_type):
    if not (factor_type in FactorType):
      print('Invalid factor type: {}'.format(factor_type))
      raise NotImplementedError

    return {
      FactorType.AND: 2,
      FactorType.XOR: 2,
      FactorType.XOR_C0: 1,
      FactorType.XOR_C1: 1,
      FactorType.OR: 2,
      FactorType.INV: 1,
      FactorType.SHIFT: 1
    }[factor_type]


class Factor(object):
  DIRECTED_GRAPH = nx.DiGraph()
  
  @staticmethod
  def reset():
    Factor.DIRECTED_GRAPH = nx.DiGraph()

  def __init__(self, factor_type, out, in1, in2=None):
    self.n_inputs = FactorType.numInputs(factor_type)
    if self.n_inputs == 2 and in2 is None:
      raise RuntimeError('Factor {} requires two inputs'.format(factor_type.value))
    elif self.n_inputs == 1 and in2 is not None:
      raise RuntimeError('{} requires 1 input'.format(factor_type.value))

    self.factor_type = factor_type
    self.out = out
    self.in1 = in1
    Factor.DIRECTED_GRAPH.add_edge(in1, out)

    if in2 is not None:
      self.in2 = in2
      Factor.DIRECTED_GRAPH.add_edge(in2, out)

  def __str__(self):
    s = '{};{};{}'.format(self.factor_type.value, self.out, self.in1)
    if self.n_inputs > 1:
      s += ';{}'.format(self.in2)
    return s
