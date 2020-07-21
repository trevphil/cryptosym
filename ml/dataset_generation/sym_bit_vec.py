# -*- coding: utf-8 -*-
from BitVector import BitVector
from copy import deepcopy

from dataset_generation.bit import Bit


class SymBitVec(object):
  def __init__(self, bits, unknown=False):
    if type(bits) is list:
      self.bits = bits
    elif type(bits) is BitVector:
      self.bits = [Bit(bits[i], unknown) for i in range(len(bits))]
    else:
      raise RuntimeError('SymBitVec: Unsupported type {}'.format(type(bits)))
  
  def rvIndices(self):
    return [bit.index for bit in self.bits if bit.is_rv]

  def __len__(self):
    return len(self.bits)
  
  def __getitem__(self, i):
    return self.bits[i]
  
  def __invert__(a):
    return SymBitVec([~a[i] for i in range(len(a))])

  def __xor__(a, b):
    assert len(a) == len(b)
    return SymBitVec([a[i] ^ b[i] for i in range(len(a))])
  
  def __and__(a, b):
    assert len(a) == len(b)
    return SymBitVec([a[i] & b[i] for i in range(len(a))])
  
  def __or__(a, b):
    assert len(a) == len(b)
    return SymBitVec([a[i] | b[i] for i in range(len(a))])
  
  def __lshift__(a, n):
    if n == 0:
      return deepcopy(a)
    output_bits = deepcopy(a.bits[n:])
    output_bits += [Bit(0, False) for _ in range(n)]
    return SymBitVec(output_bits)

  def __rshift__(a, n):
    if n == 0:
      return deepcopy(a)
    output_bits = deepcopy(a.bits[:-n])
    output_bits = [Bit(0, False) for _ in range(n)] + output_bits
    return SymBitVec(output_bits)
  