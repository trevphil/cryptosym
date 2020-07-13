# -*- coding: utf-8 -*-
import networkx as nx
from matplotlib import pyplot as plt
from BitVector import BitVector
from enum import Enum, unique

from generate_dataset import sample


@unique
class FactorType(Enum):
  AND = 1
  XOR = 2
  OR = 3
  INV = 4
  SHIFT = 5

  @staticmethod
  def numInputs(factor_type):
    if not (factor_type in FactorType):
      print('Invalid factor type: {}'.format(factor_type))
      return -1

    return {
      FactorType.AND: 2,
      FactorType.XOR: 2,
      FactorType.OR: 2,
      FactorType.INV: 1,
      FactorType.SHIFT: 1
    }[factor_type]


class Factor(object):
  DIRECTED_GRAPH = nx.DiGraph()

  def __init__(self, factor_type, out, in1, in2=None):
    n_inputs = FactorType.numInputs(factor_type)
    if n_inputs == 2 and in2 is None:
      raise RuntimeError('Factor {} requires two inputs'.format(factor_type))
    elif n_inputs == 1 and in2 is not None:
      raise RuntimeError('{} requires 1 input'.format(factor_type))

    self.factor_type = factor_type
    self.out = out
    self.in1 = in1
    Factor.DIRECTED_GRAPH.add_edge(in1, out)

    if in2 is not None:
      self.in2 = in2
      Factor.DIRECTED_GRAPH.add_edge(in2, out)

  def prob(out_val, in1_val, in2_val=None):
    if FactorType.numInputs(self.factor_type) == 2 and in2_val is None:
      raise RuntimeError('{} requires two inputs'.format(self.factor_type))
    
    if self.factor_type == FactorType.AND:
      return out_val == (i_val & j_val)
    elif self.factor_type == FactorType.XOR:
      return out_val == (i_val ^ j_val)
    elif self.factor_type == FactorType.OR:
      return out_val == (i_val | j_val)
    elif self.factor_type == FactorType.INV:
      return out_val == (~i_val)
    elif self.factor_type == FactorType.SHIFT:
      return out_val == in1_val
    else:
      raise NotImplementedError


class SymBitVec(object):
  RV_INDEX = 0
  FACTORS = []

  def __init__(self, bv):
    self.bv = bv
    size = bv.length()
    r = range(SymBitVec.RV_INDEX, SymBitVec.RV_INDEX + size)
    self.bit_indices = [i for i in r]
    SymBitVec.RV_INDEX += size

  def __len__(self):
    return self.bv.length()
  
  def __getitem__(self, i):
    return self.bit_indices[i]

  def __and__(a, b):
    # TODO - Operation with a CONSTANT should not introduce unnecessary factors
    assert len(a) == len(b)
    out = SymBitVec(a.bv & b.bv)
    f_type = FactorType.AND
    for i in range(len(a)):
      SymBitVec.FACTORS.append(Factor(f_type, out[i], a[i], b[i]))
    return out

  def __or__(a, b):
    # TODO - Operation with a CONSTANT should not introduce unnecessary factors
    assert len(a) == len(b)
    out = SymBitVec(a.bv | b.bv)
    f_type = FactorType.OR
    for i in range(len(a)):
      SymBitVec.FACTORS.append(Factor(f_type, out[i], a[i], b[i]))
    return out

  def __xor__(a, b):
    # TODO - Operation with a CONSTANT should not introduce unnecessary factors
    assert len(a) == len(b)
    out = SymBitVec(a.bv ^ b.bv)
    f_type = FactorType.XOR
    for i in range(len(a)):
      SymBitVec.FACTORS.append(Factor(f_type, out[i], a[i], b[i]))
    return out
  
  def __lshift__(a, n):
    # TODO - Bit shifting should not require creating new factors
    out = SymBitVec(a.bv.deep_copy().shift_left(n))
    f_type = FactorType.SHIFT
    for i in range(max(0, len(a) - n)):
      SymBitVec.FACTORS.append(Factor(f_type, out[i], a[n + i]))
    return out
  
  def __rshift__(a, n):
    # TODO - Bit shifting should not require creating new factors
    out = SymBitVec(a.bv.deep_copy().shift_right(n))
    f_type = FactorType.SHIFT
    for i in range(max(0, len(a) - n)):
      SymBitVec.FACTORS.append(Factor(f_type, out[n + i], a[i]))
    return out
  
  def __invert__(a):
    # TODO - Inverting should not require creating new factors
    out = SymBitVec(~a.bv)
    f_type = FactorType.INV
    for i in range(len(a)):
      SymBitVec.FACTORS.append(Factor(f_type, out[i], a[i]))
    return out

if __name__ == '__main__':
  def hashFunction(sym):
    A = SymBitVec(BitVector(intVal=0xAC32, size=32))
    B = SymBitVec(BitVector(intVal=0xFFE1, size=32))
    C = SymBitVec(BitVector(intVal=0xBF09, size=32))
    D = SymBitVec(BitVector(intVal=0xBEEF, size=32))
    a = (sym >> 0 ) ^ A
    b = (sym >> 16) ^ B
    c = (sym >> 32) ^ C
    d = (sym >> 48) ^ D
    a = (a | b)
    b = (b & c)
    c = (c ^ d)
    s = a | (b << 16) | (c << 32) | (d << 48)
    return s

  sym_bv = SymBitVec(sample(32))
  hashed = hashFunction(sym_bv)

  print('Total bits: {}'.format(SymBitVec.RV_INDEX))
  # nx.draw(Factor.DIRECTED_GRAPH, with_labels=True)
  nx.draw(Factor.DIRECTED_GRAPH, node_size=10, with_labels=False)
  plt.show()
