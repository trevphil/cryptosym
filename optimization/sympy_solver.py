# -*- coding: utf-8 -*-

import symengine.lib.symengine_wrapper as se
# import sympy

class BinarySymbol(se.Symbol):
    def __pow__(x, n):
        return x
    def __rpow__(x, n):
        return x
    def __mul__(a, b):
        if a is b:
            return a
        return super(BinarySymbol, BinarySymbol).__mul__(a, b)
    def __rmul__(a, b):
        if a is b:
            return a
        return super(BinarySymbol, BinarySymbol).__rmul__(a, b)

# '__mul__', '__rmul__'


class SympySolver(object):
    def solve(self, factors, observed, config, all_bits):
        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        n = len(rvs)

        symbols = dict()

        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = symbols[factor.input_rvs[0]]
                symbols[rv] = 1 - inp
            elif ftype == 'SAME':
                symbols[rv] = symbols[factor.input_rvs[0]]
            elif ftype == 'PRIOR':
                symbols[rv] = BinarySymbol('x%d' % rv)
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                inp1, inp2 = symbols[inp1], symbols[inp2]
                symbols[rv] = se.expand(inp1 * inp2)

        for obs_rv, obs_val in observed.items():
            print(symbols[obs_rv])
            print('\n')

        exit()