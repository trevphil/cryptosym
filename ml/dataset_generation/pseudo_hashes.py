# -*- coding: utf-8 -*-
from BitVector import BitVector

from dataset_generation.bit import Bit, saveFactors
from dataset_generation.sym_bit_vec import SymBitVec


class SymbolicHash(object):
  def __init__(self):
    self.input_rv_indices = []
    self.hash_rv_indices = []

  def numVars(self):
    return Bit.rv_index
  
  def allBits(self):
    return BitVector(bitlist=[bool(bit.val) for bit in Bit.rv_bits])

  def saveFactors(self, filename):
    saveFactors(filename)

  def __call__(self, hash_input, difficulty=None):
    """
    @parameter
     - hash_input: A BitVector
     - difficulty: An optional difficulty, between 1-64
    @return
     - The hashed input, as a BitVector
    """

    raise NotImplementedError

"""
class PseudoHash(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    Bit.reset()
    hash_input = SymBitVec(hash_input)

    n = len(hash_input)
    A = SymBitVec(BitVector(intVal=0xAC32, size=n))
    B = SymBitVec(BitVector(intVal=0xFFE1, size=n))
    C = SymBitVec(BitVector(intVal=0xBF09, size=n))
    D = SymBitVec(BitVector(intVal=0xBEEF, size=n))

    a = (hash_input >> 0 ) ^ A
    b = (hash_input >> 16) ^ B
    c = (hash_input >> 32) ^ C
    d = (hash_input >> 48) ^ D
    a = (a | b)
    b = (b & c)
    c = (c ^ d)
    h = a | (b << 16) | (c << 32) | (d << 48)
    
    self.num_hash_bits = len(h)
"""

class XorConst(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    Bit.reset()
    hash_input = SymBitVec(hash_input, unknown=True)
    self.input_rv_indices = hash_input.rvIndices()

    n = len(hash_input)    
    A = BitVector(intVal=0x4F65D4D99B70EF1B, size=n)
    A = SymBitVec(A)

    h = hash_input ^ A
    self.hash_rv_indices = h.rvIndices()


class ShiftLeft(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    Bit.reset()
    hash_input = SymBitVec(hash_input, unknown=True)
    self.input_rv_indices = hash_input.rvIndices()
    h = hash_input << (len(hash_input) // 2)
    self.hash_rv_indices = h.rvIndices()


class ShiftRight(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    Bit.reset()
    hash_input = SymBitVec(hash_input, unknown=True)
    self.input_rv_indices = hash_input.rvIndices()
    h = hash_input >> (len(hash_input) // 2)
    self.hash_rv_indices = h.rvIndices()


class Invert(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    Bit.reset()
    hash_input = SymBitVec(hash_input, unknown=True)
    self.input_rv_indices = hash_input.rvIndices()
    h = ~hash_input
    self.hash_rv_indices = h.rvIndices()
