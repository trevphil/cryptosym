# -*- coding: utf-8 -*-

from multiprocessing import cpu_count

import gurobipy as gp
from gurobipy import GRB


class GurobiMILPSolver(object):
    def solve(self, factors, observed, config, all_bits):
        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        n = len(rvs)

        # https://www.gurobi.com/documentation/9.0/quickstart_mac/py_example_mip1_py.html
        model = gp.Model('hash_reversal')
        model.setParam(GRB.Param.Threads, cpu_count())
        model.setParam(GRB.Param.MIPFocus, 1)
        model.setParam(GRB.Param.SolutionLimit, 1)
        model.setParam(GRB.Param.Heuristics, 0.5)

        rv2var = {rv: model.addVar(vtype=GRB.BINARY, name=str(rv)) for rv in rvs}
        for rv, val in observed.items():
            model.addConstr(rv2var[rv] == bool(val))
        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = factor.input_rvs[0]
                model.addConstr(rv2var[inp] == 1 - rv2var[rv])
            elif ftype == 'SAME':
                inp = factor.input_rvs[0]
                model.addConstr(rv2var[inp] == rv2var[rv])
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                # Linearization of out = inp1 * inp2 for binary variables
                model.addConstr(rv2var[rv] <= rv2var[inp1])
                model.addConstr(rv2var[rv] <= rv2var[inp2])
                model.addConstr(rv2var[rv] >= rv2var[inp1] + rv2var[inp2] - 1)

        model.setObjective(sum(rv2var.values()), GRB.MAXIMIZE)
        model.optimize()
        solution = {rv: rv2var[rv].x for rv in rvs}
        return solution
