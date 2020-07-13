# -*- coding: utf-8 -*-
from BitVector import BitVector

from dataset_generation.symbolic import SymBitVec, saveFactors


class SymbolicHash(object):
  def __init__(self):
    self.num_hash_bits = -1

  def numVars(self):
    return SymBitVec.RV_INDEX
  
  def numHashBits(self):
    return self.num_hash_bits
  
  def bits(self):
    return SymBitVec.BITS

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


class PseudoHash(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    SymBitVec.reset()
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


class XorHashConst(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    pass


class ShiftLeft(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    pass


class ShiftRight(SymbolicHash):
  def __call__(self, hash_input, difficulty=None):
    pass
