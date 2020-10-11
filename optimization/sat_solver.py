# -*- coding: utf-8 -*-
from time import time
from ortools.sat.python import cp_model


class SatSolver(object):
    def __init__(self):
        pass

    def print_solver_info(self, solver):
        info = 'Solver info:\n'
        info += solver.ResponseStats()
        print(info)

    def solve(self, factors, observed, config, all_bits):
        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        n = len(rvs)

        model = cp_model.CpModel()
        X = lambda i: 'x%d' % i
        rv2var = {rv: model.NewBoolVar(X(rv)) for rv in rvs}
        for rv, val in observed.items():
            model.Add(rv2var[rv] == bool(val))
        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = factor.input_rvs[0]
                model.Add(rv2var[inp] != rv2var[rv])
            elif ftype == 'SAME':
                inp = factor.input_rvs[0]
                model.Add(rv2var[inp] == rv2var[rv])
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                # Linearization of out = inp1 * inp2 for binary variables
                model.Add(rv2var[rv] <= rv2var[inp1])
                model.Add(rv2var[rv] <= rv2var[inp2])
                model.Add(rv2var[rv] >= rv2var[inp1] + rv2var[inp2] - 1)
        solver = cp_model.CpSolver()
        print('Solving (n=%d)...' % len(rvs))
        start = time()
        status = solver.Solve(model)
        self.print_solver_info(solver)
        if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
            raise RuntimeError('No solution found!')
        solution = {rv: solver.Value(rv2var[rv]) for rv in rvs}
        print('Solved in %.2f s' % (time() - start))
        return solution
