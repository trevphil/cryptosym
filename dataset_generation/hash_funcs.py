# -*- coding: utf-8 -*-

import numpy as np
from BitVector import BitVector

from dataset_generation import nsha256
from dataset_generation.bit import Bit, saveFactors
from dataset_generation.factor import Factor
from dataset_generation.sym_bit_vec import SymBitVec


class SymbolicHash(object):
    def __init__(self):
        self.hash_rv_indices = []
        self.ignorable = None

    def hashIndices(self):
        return self.hash_rv_indices

    def bitsPerSample(self):
        return len(Bit.rv_bits)

    def allBits(self):
        return BitVector(bitlist=[bool(bit.val) for bit in Bit.rv_bits])

    def numUsefulFactors(self):
        self.ignorable = self.findIgnorableRVs()
        assert len(Bit.factors) == len(Bit.rv_bits)
        return len(Bit.factors) - len(self.ignorable)

    def saveFactors(self, filename):
        saveFactors(filename, self.ignorable)

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
        hash_input = SymBitVec(hash_input, unknown=True)
        h = self.hash(hash_input, difficulty)
        self.hash_rv_indices = h.rvIndices()
        return h

    def findIgnorableRVs(self):
        idx, num_bits = 0, self.bitsPerSample()
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

    def forwardPropagateInput(self, hash_input):
        if self.ignorable is None:
            self.ignorable = self.findIgnorableRVs()

        bitvals = dict()
        for factor in Bit.factors:
            rv = factor.out.index
            if rv in self.ignorable:
                continue
            ftype = factor.factor_type.value
            if ftype == 'PRIOR':
                bitvals[rv] = float(hash_input[rv])
            elif ftype == 'INV':
                inp = factor.inputs[0].index
                bitvals[rv] = 1 - bitvals[inp]
            elif ftype == 'AND':
                inp1 = factor.inputs[0].index
                inp2 = factor.inputs[1].index
                bitvals[rv] = inp1 * inp2
            else:
                raise RuntimeError(ftype)
        fwd = BitVector(size=len(self.hash_rv_indices))
        for i, rv in enumerate(self.hash_rv_indices):
            fwd[i] = bool(bitvals[rv])
        return fwd


class SHA256Hash(SymbolicHash):
    def hash(self, hash_input, difficulty):
        bitvecs = nsha256.sha256(
            hash_input, difficulty=difficulty).bitvecDigest()
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
        A = SymBitVec(BitVector(intVal=A, size=n))
        B = SymBitVec(BitVector(intVal=B, size=n))
        C = SymBitVec(BitVector(intVal=C, size=n))
        D = SymBitVec(BitVector(intVal=D, size=n))

        mask = SymBitVec(BitVector(intVal=(1 << n4) - 1, size=n))
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
        A = SymBitVec(BitVector(intVal=A, size=n))
        B = SymBitVec(BitVector(intVal=B, size=n))
        C = SymBitVec(BitVector(intVal=C, size=n))
        D = SymBitVec(BitVector(intVal=D, size=n))

        mask = SymBitVec(BitVector(intVal=(1 << n4) - 1, size=n))
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
        A = SymBitVec(BitVector(intVal=0x4F65D4D99B70EF1B, size=n))
        return hash_input + A


class Add(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        half_n = n // 2
        mask = SymBitVec(BitVector(intVal=(1 << half_n) - 1, size=n))
        a = (hash_input & mask)
        b = (hash_input >> half_n) & mask
        out = (a + b) & mask
        inv_input = ~hash_input
        h = (inv_input & (mask << half_n)) | out
        return h


class XorConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(BitVector(intVal=0x4F65D4D99B70EF1B, size=n))
        return hash_input ^ A


class AndConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(BitVector(intVal=0x4F65D4D99B70EF1B, size=n))
        return hash_input & A


class OrConst(SymbolicHash):
    def hash(self, hash_input, difficulty):
        n = len(hash_input)
        A = SymBitVec(BitVector(intVal=0x4F65D4D99B70EF1B, size=n))
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
