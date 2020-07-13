# -*- coding: utf-8 -*-
from BitVector import BitVector

from dataset_generation.factor import Factor, FactorType


class SymBitVec(object):
  RV_INDEX = 0
  FACTORS = []
  BITS = BitVector(size=0)

  @staticmethod
  def reset():
    SymBitVec.RV_INDEX = 0
    SymBitVec.FACTORS = []
    SymBitVec.BITS = BitVector(size=0)
    Factor.reset()

  def __init__(self, bv):
    self.bv = bv
    size = bv.length()
    r = range(SymBitVec.RV_INDEX, SymBitVec.RV_INDEX + size)
    self.bit_indices = [i for i in r]
    SymBitVec.RV_INDEX += size
    SymBitVec.BITS = SymBitVec.BITS + bv

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


def saveFactors(filename):
  with open(filename, 'w') as f:
    for factor in SymBitVec.FACTORS:
      f.write(str(factor) + '\n')
