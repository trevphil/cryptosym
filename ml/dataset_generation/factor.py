# -*- coding: utf-8 -*-
import networkx as nx
from enum import Enum, unique

@unique
class FactorType(Enum):
  AND = 'AND'
  XOR = 'XOR'
  OR = 'OR'
  INV = 'INV'

  @staticmethod
  def numInputs(factor_type):
    if not (factor_type in FactorType):
      raise NotImplementedError('Invalid factor type: {}'.format(factor_type))

    return {
      FactorType.AND: 2,
      FactorType.XOR: 2,
      FactorType.OR: 2,
      FactorType.INV: 1,
    }[factor_type]


class Factor(object):
  directed_graph = nx.DiGraph()

  @staticmethod
  def reset():
    Factor.directed_graph = nx.DiGraph()

  def __init__(self, factor_type, out, inputs=[]):
    self.n_inputs = FactorType.numInputs(factor_type)
    if self.n_inputs != len(inputs):
      err_msg = 'Factor {} requires {} input(s), given {}'.format(factor_type.value, self.n_inputs, len(inputs))
      raise RuntimeError(err_msg)

    self.factor_type = factor_type
    self.out = out
    self.inputs = inputs
    for inp in inputs:
      Factor.directed_graph.add_edge(inp.index, out.index)

  def __str__(self):
    if len(self.inputs) == 0:
      return '{};{}'.format(self.factor_type.value, self.out.index)
    else:
      input_str = ';'.join(str(inp.index) for inp in self.inputs)
      return '{};{};{}'.format(self.factor_type.value, self.out.index, input_str)
