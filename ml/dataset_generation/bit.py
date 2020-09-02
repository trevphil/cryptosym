# -*- coding: utf-8 -*-
from dataset_generation.factor import Factor, FactorType


class Bit(object):
    rv_index = 0
    factors = []
    rv_bits = []

    def __init__(self, bit_value, is_rv, is_prior=False):
        self.val = bool(bit_value)
        self.is_rv = is_rv
        if self.is_rv:
            self.index = Bit.rv_index
            Bit.rv_index += 1
            Bit.rv_bits.append(self)
            if is_prior:
                Bit.factors.append(Factor(FactorType.PRIOR, self))

    @staticmethod
    def reset():
        Bit.rv_index = 0
        Bit.factors = []
        Bit.rv_bits = []

    def __repr__(self):
        if self.is_rv:
            return 'Bit({}, index: {})'.format(int(self.val), self.index)
        else:
            return 'Bit({}, constant)'.format(int(self.val))

    def __invert__(a):
        result_val = not bool(a.val)
        is_rv = a.is_rv
        result = Bit(result_val, is_rv)

        if a.is_rv:
            Bit.factors.append(Factor(FactorType.INV, result, [a]))

        return result

    def __xor__(a, b):
        result_val = a.val ^ b.val

        if a.is_rv and b.is_rv:
            tmp1 = ~(a & b)
            tmp2 = ~(a & tmp1)
            tmp3 = ~(b & tmp1)
            return ~(tmp2 & tmp3)
        elif a.is_rv:
            if b.val is False:
                # XOR with a constant of 0 is simply the other input
                return a
            else:
                # XOR with a constant of 1 is the inverse of the other input
                return ~a
        elif b.is_rv:
            if a.val is False:
                # XOR with a constant of 0 is simply the other input
                return b
            else:
                # XOR with a constant of 1 is the inverse of the other input
                return ~b
        else:
            # Both are constants
            return Bit(result_val, False)

    def __or__(a, b):
        result_val = a.val | b.val

        # If OR-ing with a constant of 1, result will be always be 1
        if not a.is_rv and a.val is True:
            return Bit(result_val, False)
        elif not b.is_rv and b.val is True:
            return Bit(result_val, False)

        if a.is_rv and b.is_rv:
            tmp1 = ~(a & a)
            tmp2 = ~(b & b)
            return ~(tmp1 & tmp2)
        elif a.is_rv:
            # Here, "b" is a constant equal to 0, so result is directly "a"
            return a
        elif b.is_rv:
            # Here, "a" is a constant equal to 0, so result is directly "b"
            return b
        else:
            # Both are constants
            return Bit(result_val, False)

    def __and__(a, b):
        result_val = a.val & b.val

        # If AND-ing with a constant of 0, result will always be 0
        if not a.is_rv and a.val is False:
            return Bit(result_val, False)
        elif not b.is_rv and b.val is False:
            return Bit(result_val, False)

        if a.is_rv and b.is_rv:
            if a.index == b.index:
                # Special logic to prevent same RVs as inputs to AND factor
                tmp = Bit(b.val, b.is_rv)
                Bit.factors.append(Factor(FactorType.SAME, tmp, [b]))
                b = tmp
            result = Bit(result_val, True)
            Bit.factors.append(Factor(FactorType.AND, result, [a, b]))
            return result
        elif a.is_rv:
            # Here, "b" is a constant equal to 1, so result is directly "a"
            return a
        elif b.is_rv:
            # Here, "a" is a constant equal to 1, so result is directly "b"
            return b
        else:
            # Both are constants
            return Bit(result_val, False)

    @staticmethod
    def add(a, b, carry_in=None):
        if carry_in is None:
            carry_in = Bit(0, False)

        sum1 = a ^ b
        carry1 = a & b

        sum2 = carry_in ^ sum1
        carry2 = carry_in & sum1

        carry_out = carry1 | carry2
        return sum2, carry_out


def saveFactors(filename, ignore):
    with open(filename, 'w') as f:
        for factor in Bit.factors:
            if factor.out.index not in ignore:
                f.write(str(factor) + '\n')
