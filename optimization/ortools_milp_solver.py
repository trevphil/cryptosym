# -*- coding: utf-8 -*-
from multiprocessing import cpu_count
from ortools.linear_solver import pywraplp


class OrtoolsMILPSolver(object):
    def __init__(self):
        pass

    def solve(self, factors, observed, config, all_bits):
        # https://developers.google.com/optimization/mip/integer_opt
        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        n = len(rvs)

        # ptype = pywraplp.Solver.BOP_INTEGER_PROGRAMMING
        # ptype = pywraplp.Solver.CBC_MIXED_INTEGER_PROGRAMMING
        ptype = pywraplp.Solver.SAT_INTEGER_PROGRAMMING

        solver = pywraplp.Solver('CBC', ptype)
        solver.SetNumThreads(cpu_count())
        X = lambda i: 'x%d' % i
        rv2var = {rv: solver.BoolVar(X(rv)) for rv in rvs}
        for rv, val in observed.items():
            solver.Add(rv2var[rv] == bool(val))
        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = factor.input_rvs[0]
                solver.Add(rv2var[inp] == 1 - rv2var[rv])
            elif ftype == 'SAME':
                inp = factor.input_rvs[0]
                solver.Add(rv2var[inp] == rv2var[rv])
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                # Linearization of out = inp1 * inp2 for binary variables
                solver.Add(rv2var[rv] <= rv2var[inp1])
                solver.Add(rv2var[rv] <= rv2var[inp2])
                solver.Add(rv2var[rv] >= rv2var[inp1] + rv2var[inp2] - 1)

        solver.Maximize(list(rv2var.values())[0])
        print('Solving (n=%d)...' % len(rvs))
        _ = solver.Solve()
        solve_time = solver.wall_time() / 1000.0
        print('Problem solved in %.2f seconds' % solve_time)
        solution = {rv: rv2var[rv].solution_value() for rv in rvs}
        return solution
