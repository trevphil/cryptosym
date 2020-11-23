# -*- coding: utf-8 -*-

import torch
import numpy as np
from BitVector import BitVector

from dataset_generation import nsha256
from dataset_generation.bit import Bit, save_factors
from dataset_generation.factor import Factor
from dataset_generation.sym_bit_vec import SymBitVec


def hash_algorithms():
    return {
        'sha256': SHA256Hash(),
        'lossyPseudoHash': LossyPseudoHash(),
        'nonLossyPseudoHash': NonLossyPseudoHash(),
        'xorConst': XorConst(),
        'shiftLeft': ShiftLeft(),
        'shiftRight': ShiftRight(),
        'invert': Invert(),
        'andConst': AndConst(),
        'orConst': OrConst(),
        'addConst': AddConst(),
        'add': Add(),
    }


class SymbolicHash(object):
    def __init__(self):
        self.hash_rv_indices = []
        self.ignorable = None

    def hash_indices(self):
        return self.hash_rv_indices

    def bits_per_sample(self):
        return len(Bit.rv_bits)

    def all_bits(self):
        return BitVector(bitlist=[bool(bit.val) for bit in Bit.rv_bits])

    def num_useful_factors(self):
        if self.ignorable is None:
            self.ignorable = self.find_ignorable_rvs()
        assert len(Bit.factors) == len(Bit.rv_bits)
        return len(Bit.factors) - len(self.ignorable)

    def save_factors(self, factor_file, cnf_file, graphml_file):
        if self.ignorable is None:
            self.ignorable = self.find_ignorable_rvs()
        save_factors(factor_file, cnf_file, graphml_file, self.ignorable)

    def hash(self, hash_input, difficulty):
        raise NotImplementedError  # Override in sub-classes

    def __call__(self, hash_input, difficulty):
        """
        @parameter
         - hash_input: A BitVector
         - difficulty: An optional difficulty, between 1-64 inclusive
        @return
         - The hashed input, as a SymBitVec
        """

        Bit.reset()
        Factor.reset()
        SymBitVec.tensor_mode = isinstance(hash_input, torch.Tensor)
        hash_input = SymBitVec(hash_input, unknown=True)
        h = self.hash(hash_input, difficulty)
        if not SymBitVec.tensor_mode:
            self.hash_rv_indices = h.rv_indices()
        return h

    def find_ignorable_rvs(self):
        idx, num_bits = 0, self.bits_per_sample()
        seen, ignorable = set(), set(range(num_bits))
        queue = list(self.hash_rv_indices)
        while idx < len(queue):
            rv = queue[idx]  # Do not pop from queue, to speed it up
            ignorable.discard(rv)
            seen.add(rv)
            factor = Bit.factors[rv]
            parents = [inp.index for inp in factor.inputs]
            queue += [p for p in parents if not p in seen]
            idx += 1
        return ignorable


class SHA256Hash(SymbolicHash):
    def hash(self, hash_input, difficulty):
        bitvecs = nsha256.sha256(
            hash_input, difficulty=difficulty).bitvec_digest()
        result = bitvecs[0]
        for bv in bitvecs[1:]:
            result = result.concat(bv)
        return result


class LossyPseudoHash(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        n4 = n // 4

        np.random.seed(1)
        A = int.from_bytes(np.random.bytes(n // 8), 'big')
        B = int.from_bytes(np.random.bytes(n // 8), 'big')
        C = int.from_bytes(np.random.bytes(n // 8), 'big')
        D = int.from_bytes(np.random.bytes(n // 8), 'big')
        A = SymBitVec(A, size=n)
        B = SymBitVec(B, size=n)
        C = SymBitVec(C, size=n)
        D = SymBitVec(D, size=n)

        mask = SymBitVec((1 << n4) - 1, size=n)
        h = hash_input

        for _ in range(difficulty):
            a = ((h >> (n4 * 0)) & mask) ^ A
            b = ((h >> (n4 * 1)) & mask) ^ B
            c = ((h >> (n4 * 2)) & mask) ^ C
            d = ((h >> (n4 * 3)) & mask) ^ D
            a = (a | b)
            b = (b & c)
            c = (c ^ d)
            h = a | (b << (n4 * 1)) | (c << (n4 * 2)) | (d << (n4 * 3))
        return h


class NonLossyPseudoHash(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        n4 = n // 4

        np.random.seed(1)
        A = int.from_bytes(np.random.bytes(n // 8), 'big')
        B = int.from_bytes(np.random.bytes(n // 8), 'big')
        C = int.from_bytes(np.random.bytes(n // 8), 'big')
        D = int.from_bytes(np.random.bytes(n // 8), 'big')
        A = SymBitVec(A, size=n)
        B = SymBitVec(B, size=n)
        C = SymBitVec(C, size=n)
        D = SymBitVec(D, size=n)

        mask = SymBitVec((1 << n4) - 1, size=n)
        h = hash_input

        for _ in range(difficulty):
            a = ((h >> (n4 * 0)) & mask) ^ A
            b = ((h >> (n4 * 1)) & mask) ^ B
            c = ((h >> (n4 * 2)) & mask) ^ C
            d = ((h >> (n4 * 3)) & mask) ^ D
            h = a | (b << (n4 * 1)) | (c << (n4 * 2)) | (d << (n4 * 3))
        return h


class AddConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(0x4F65D4D99B70EF1B, size=n)
        return hash_input + A


class Add(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        half_n = n // 2
        mask = SymBitVec((1 << half_n) - 1, size=n)
        a = (hash_input & mask)
        b = (hash_input >> half_n) & mask
        out = (a + b) & mask
        inv_input = ~hash_input
        h = (inv_input & (mask << half_n)) | out
        return h


class XorConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(0x4F65D4D99B70EF1B, size=n)
        return hash_input ^ A


class AndConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(0x4F65D4D99B70EF1B, size=n)
        return hash_input & A


class OrConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(0x4F65D4D99B70EF1B, size=n)
        return hash_input | A


class ShiftLeft(SymbolicHash):
    def hash(self, hash_input, difficulty):
        return hash_input << (len(hash_input) // 2)


class ShiftRight(SymbolicHash):
    def hash(self, hash_input, difficulty):
        return hash_input >> (len(hash_input) // 2)


class Invert(SymbolicHash):
    def hash(self, hash_input, difficulty):
        return ~hash_input
