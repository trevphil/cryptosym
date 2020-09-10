# -*- coding: utf-8 -*-
import numpy as np

"""
See: https://arxiv.org/pdf/1909.08605.pdf
"""


class GNC(object):
    def __init__(self, c, verbose=True):
        self._c_sq = c * c
        self._verbose = verbose
        self._mu = None
        self._itr = 0

    def mu(self, residuals_sq):
        if self._mu is None:
            self._mu = 2 * np.max(residuals_sq) / self._c_sq
            if self._verbose:
                print('mu initialized to %.3f' % self._mu)
        return max(1.0, self._mu)

    def iteration(self):
        return self._itr

    def increment(self):
        self._itr += 1
        if self._mu is not None:
            self._mu = max(1.0, self._mu / 1.4)
            if self._verbose:
                print('mu is now %.3f' % self._mu)
        print('Finished iteration %d' % self._itr)
