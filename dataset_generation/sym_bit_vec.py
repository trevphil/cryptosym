# -*- coding: utf-8 -*-

import torch
from BitVector import BitVector
from copy import deepcopy

from dataset_generation.bit import Bit


class SymBitVec(object):
    tensor_mode = False

    def __init__(self, bits, size=None, unknown=False):
        self.bits = None
        self.is_tensor = False

        if isinstance(bits, list):
            self.bits = bits
        elif isinstance(bits, int):
            if size is None:
                err = '"size" must be provided when initializing with int'
                raise RuntimeError(err)
            if SymBitVec.tensor_mode:
                bits = self._int_to_tensor(bits, size, unknown)
            else:
                bits = BitVector(intVal=bits, size=size)

        if isinstance(bits, torch.Tensor):
            self.bits = bits
            self.is_tensor = True
        elif isinstance(bits, BitVector):
            self.bits = [Bit(bits[i], unknown, unknown)
                         for i in range(len(bits))]

        if self.bits is None:
            err = 'SymBitVec: Unsupported type {}'.format(type(bits))
            raise RuntimeError(err)

    def _int_to_tensor(self, num, size, requires_grad):
        result = torch.zeros(size, requires_grad=requires_grad)
        for i in range(size):
            result[size - i - 1] = int((num >> i) & 1)
        return result

    def rv_indices(self):
        if self.is_tensor:
            raise NotImplementedError
        return [bit.index for bit in self.bits if bit.is_rv]

    def hex(self):
        return hex(int(self))[2:].lower()

    def concat(self, other):
        if self.is_tensor:
            new_bits = torch.cat((self.bits, other.bits), axis=0)
        else:
            new_bits = self.bits + other.bits
        return SymBitVec(new_bits)

    def extract(self, lower_bound, upper_bound):
        new_bits = self.bits[lower_bound:upper_bound]
        return SymBitVec(new_bits)

    def resize(self, n):
        if self.is_tensor:
            return self._resize_tensor(n)
        elif n == len(self):
            return self
        elif n < len(self):
            return SymBitVec(self.bits[-n:]) \
                if n > 0 else SymBitVec([])
        else:
            num_zeros = n - len(self)
            zeros = [Bit(0, False) for _ in range(num_zeros)]
            return SymBitVec(zeros + self.bits)

    def _resize_tensor(self, n):
        if n == len(self):
            return self
        elif n < len(self):
            empty = self.bits.new_zeros(0, requires_grad=False)
            return SymBitVec(self.bits[-n:]) \
                if n > 0 else SymBitVec(empty)
        else:
            nz = n - len(self)
            zeros = self.bits.new_zeros(nz, requires_grad=False)
            return SymBitVec(torch.cat((zeros, self.bits), axis=0))

    def __len__(self):
        if self.is_tensor:
            return self.bits.size(0)
        return len(self.bits)

    def __getitem__(self, i):
        if self.is_tensor:
            n = len(self) - 1
            return self.bits[n - i].item()
        return self.bits[i]

    def __int__(self):
        n = len(self)
        if self.is_tensor:
            result = 0
            for i in range(n):
                result += (int(self[i]) << i)
            return result

        bv = BitVector(size=n)
        for i in range(n):
            bv[i] = self.bits[i].val
        return int(bv)

    def __invert__(a):
        if a.is_tensor:
            return SymBitVec(1.0 - a.bits)
        return SymBitVec([~a[i] for i in range(len(a))])

    def __xor__(a, b):
        assert len(a) == len(b)
        if a.is_tensor and b.is_tensor:
            tmp1 = ~(a & b)
            tmp2 = ~(a & tmp1)
            tmp3 = ~(b & tmp1)
            return ~(tmp2 & tmp3)
        return SymBitVec([a[i] ^ b[i] for i in range(len(a))])

    def __and__(a, b):
        assert len(a) == len(b)
        if a.is_tensor and b.is_tensor:
            return SymBitVec(a.bits * b.bits)
        return SymBitVec([a[i] & b[i] for i in range(len(a))])

    def __or__(a, b):
        assert len(a) == len(b)
        if a.is_tensor and b.is_tensor:
            return ~((~a) & (~b))
        return SymBitVec([a[i] | b[i] for i in range(len(a))])

    def __lshift__(a, n):
        if n == 0:
            return a if a.is_tensor else deepcopy(a)
        if a.is_tensor:
            zeros = a.bits.new_zeros(n, requires_grad=False)
            return SymBitVec(torch.cat((a.bits[n:], zeros), axis=0))
        output_bits = deepcopy(a.bits[n:])
        output_bits += [Bit(0, False) for _ in range(n)]
        return SymBitVec(output_bits)

    def __rshift__(a, n):
        if n == 0:
            return a if a.is_tensor else deepcopy(a)
        if a.is_tensor:
            zeros = a.bits.new_zeros(n, requires_grad=False)
            return SymBitVec(torch.cat((zeros, a.bits[:-n]), axis=0))
        output_bits = deepcopy(a.bits[:-n])
        output_bits = [Bit(0, False) for _ in range(n)] + output_bits
        return SymBitVec(output_bits)

    def __add__(a, b):
        n = len(a)
        assert n == len(b)

        if a.is_tensor and b.is_tensor:
            return SymBitVec.add_tensors(a, b)

        carry = None
        output_bits = []
        for i in reversed(range(n)):
            out, carry = Bit.add(a[i], b[i], carry)
            output_bits = [out] + output_bits
        return SymBitVec(output_bits)

    @staticmethod
    def add_tensors(a, b):
        a = SymBitVec(a.bits.clone())
        b = SymBitVec(b.bits.clone())
        zero = torch.zeros(1, requires_grad=False)
        while torch.sum(b.bits) > 0:
            carry = a & b
            a = a ^ b
            b.bits[:-1] = carry.bits[1:]
            b.bits[-1] = zero
        return a
