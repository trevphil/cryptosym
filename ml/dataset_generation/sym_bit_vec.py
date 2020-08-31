# -*- coding: utf-8 -*-
from BitVector import BitVector
from copy import deepcopy

from dataset_generation.bit import Bit


class SymBitVec(object):
    def __init__(self, bits, unknown=False):
        if isinstance(bits, list):
            self.bits = bits
        elif isinstance(bits, BitVector):
            self.bits = [Bit(bits[i], unknown, unknown)
                         for i in range(len(bits))]
        else:
            raise RuntimeError(
                'SymBitVec: Unsupported type {}'.format(
                    type(bits)))

    def rvIndices(self):
        return [bit.index for bit in self.bits if bit.is_rv]

    def hex(self):
        return hex(int(self))[2:].lower()

    def concat(self, other):
        new_bits = self.bits + other.bits
        return SymBitVec(new_bits)

    def extract(self, lower_bound, upper_bound):
        new_bits = self.bits[lower_bound:upper_bound]
        return SymBitVec(new_bits)

    def resize(self, new_size):
        if new_size == len(self):
            return self
        elif new_size < len(self):
            return SymBitVec(self.bits[-new_size:]
                             ) if new_size > 0 else SymBitVec([])
        else:
            num_zeros = new_size - len(self)
            return SymBitVec([Bit(0, False)
                              for _ in range(num_zeros)] + self.bits)

    def __len__(self):
        return len(self.bits)

    def __getitem__(self, i):
        return self.bits[i]

    def __int__(self):
        n = len(self)
        bv = BitVector(size=n)
        for i in range(n):
            bv[i] = self.bits[i].val
        return int(bv)

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

    def __add__(a, b):
        n = len(a)
        assert n == len(b)
        carry = None
        output_bits = []
        for i in reversed(range(n)):
            out, carry = Bit.add(a[i], b[i], carry)
            output_bits = [out] + output_bits
        return SymBitVec(output_bits)
