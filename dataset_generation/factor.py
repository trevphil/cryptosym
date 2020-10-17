# -*- coding: utf-8 -*-

import sympy
import networkx as nx
from enum import Enum, unique


@unique
class FactorType(Enum):
    PRIOR = 'PRIOR'
    INV = 'INV'
    SAME = 'SAME'
    AND = 'AND'

    @staticmethod
    def num_inputs(factor_type):
        if not (factor_type in FactorType):
            raise NotImplementedError(
                'Invalid factor type: {}'.format(factor_type))

        return {
            FactorType.PRIOR: 0,
            FactorType.INV: 1,
            FactorType.SAME: 1,
            FactorType.AND: 2,
        }[factor_type]


class Factor(object):
    directed_graph = nx.DiGraph()

    @staticmethod
    def reset():
        Factor.directed_graph = nx.DiGraph()

    def __init__(self, factor_type, out, inputs=[]):
        self.n_inputs = FactorType.num_inputs(factor_type)
        if self.n_inputs != len(inputs):
            err_msg = 'Factor {} requires {} input(s), given {}'.format(
                factor_type.value, self.n_inputs, len(inputs))
            raise RuntimeError(err_msg)

        self.factor_type = factor_type
        self.out = out
        self.inputs = inputs
        if len(inputs) > 0:
            for inp in inputs:
                Factor.directed_graph.add_edge(inp.index, out.index)
        else:
            Factor.directed_graph.add_node(out.index)

    def __str__(self):
        if len(self.inputs) == 0:
            return '{};{}'.format(self.factor_type.value, self.out.index)
        else:
            input_str = ';'.join(str(inp.index) for inp in self.inputs)
            return '{};{};{}'.format(
                self.factor_type.value, self.out.index, input_str)

    def cnf(self, rv2idx):
        X = lambda i: 'x%d' % (rv2idx[i] + 1)  # 1-indexed, not 0-indexed
        out = sympy.Symbol(X(self.out.index))
        inp = [sympy.Symbol(X(i.index)) for i in self.inputs]

        if self.factor_type == FactorType.INV:
            s = str(sympy.to_cnf(sympy.Equivalent(out, ~inp[0])))
        elif self.factor_type == FactorType.SAME:
            s = str(sympy.to_cnf(sympy.Equivalent(out, inp[0])))
        elif self.factor_type == FactorType.AND:
            s = str(sympy.to_cnf(sympy.Equivalent(inp[0] & inp[1], out)))
        else:
            err = 'No supported conjunctive normal form for %s' % self.factor_type
            raise NotImplementedError(err)

        s = s.replace('~', '-').replace(' & ', '\n').replace('(', '').replace(')', '')
        s = s.replace('x', '').replace(' |', '')
        clauses = s.split('\n')
        return [clause + ' 0' for clause in clauses]
