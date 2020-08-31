# -*- coding: utf-8 -*-

"""
SAME: (i - j) ^ 2
  first order:
    d/di = 2i - 2j
    d/dj = 2j - 2i
  second order:
    d/di,di = 2
    d/di,dj = -2
    d/dj,di = -2
    d/dj,dj = 2

INV: (1 - i - j) ^ 2
  first order:
    d/di = 2i + 2j - 2
    d/dj = 2j + 2i - 2
  second order:
    d/di,di = 2
    d/di,dj = 2
    d/dj,di = 2
    d/dj,dj = 2

AND: (ij - k) ^ 2
  first order:
    d/di = 2ij^2 - 2jk
    d/dj = 2i^2j - 2ik
    d/dk = 2k - 2ij
  second order:
    d/di,di = 2j^2
    d/di,dj = 4ij - 2k
    d/di,dk = -2j
    d/dj,di = 4ij - 2k
    d/dj,dj = 2i^2
    d/dj,dk = -2i
    d/dk,di = -2j
    d/dk,dj = -2i
    d/dk,dk = 2
"""


class Factor(object):
    def __init__(self, data_string):
        parts = data_string.strip().split(';')
        self.factor_type = parts[0]
        self.output_rv = int(parts[1])
        self.input_rvs = []
        for p in parts[2:]:
            self.input_rvs.append(int(p))
        if self.factor_type == 'AND' and len(self.input_rvs) < 2:
            print('Warning: AND factors has %d inputs' % len(self.input_rvs))
        self.referenced_rvs = set(self.input_rvs + [self.output_rv])

    def _has_variable(self, variable):
        return variable in self.referenced_rvs

    def first_order(self, with_respect_to, x):
        wrt = with_respect_to
        if not self._has_variable(wrt):
            raise RuntimeError

        if self.factor_type == 'SAME':
            i, j = self.input_rvs[0], self.output_rv
            if wrt == i:
                return 2 * (x[i] - x[j])
            else:
                return 2 * (x[j] - x[i])
        elif self.factor_type == 'INV':
            i, j = self.input_rvs[0], self.output_rv
            return 2 * (x[i] + x[j] - 1)
        elif self.factor_type == 'AND':
            i, j = self.input_rvs
            k = self.output_rv
            if wrt == i:
                return 2 * (x[i] * x[j] * x[j] - x[j] * x[k])
            elif wrt == j:
                return 2 * (x[i] * x[i] * x[j] - x[i] * x[k])
            else:
                return 2 * (x[k] - x[i] * x[j])
        elif self.factor_type == 'PRIOR':
            return 0.0
        else:
            raise RuntimeError('Unsupported factor')

    def second_order(self, first, second, x):
        if not self._has_variable(first):
            raise RuntimeError
        if not self._has_variable(second):
            raise RuntimeError

        pair = (first, second)
        if self.factor_type == 'SAME':
            i, j = self.input_rvs[0], self.output_rv
            if pair == (i, i) or pair == (j, j):
                return 2.0
            else:
                return -2.0
        elif self.factor_type == 'INV':
            return 2.0
        elif self.factor_type == 'AND':
            i, j = self.input_rvs
            k = self.output_rv
            if pair == (i, i):
                return 2 * x[j] * x[j]
            elif pair == (i, j) or pair == (j, i):
                return 4 * x[i] * x[j] - 2 * x[k]
            elif pair == (i, k):
                return -2 * x[j]
            elif pair == (j, j):
                return 2 * x[i] * x[i]
            elif pair == (j, k):
                return -2 * x[i]
            elif pair == (k, i):
                return -2 * x[j]
            elif pair == (k, j):
                return -2 * x[i]
            else:
                return 2.0
        elif self.factor_type == 'PRIOR':
            return 0.0
        else:
            raise RuntimeError('Unsupported factor')
