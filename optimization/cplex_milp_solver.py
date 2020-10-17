# -*- coding: utf-8 -*-

from multiprocessing import cpu_count
from docplex.mp.model import Model


class CplexMILPSolver(object):
    def __init__(self):
        pass

    def solve(self, factors, observed, config, all_bits):
        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        rv2idx = {rv: i for i, rv in enumerate(rvs)}
        n = len(rvs)

        # https://cdn.rawgit.com/IBMDecisionOptimization/docplex-doc/master/docs/mp/docplex.mp.model.html
        model = Model(name='hash_reversal')
        model.context.cplex_parameters.threads = cpu_count()
        rv2var = {rv: model.binary_var(str(rv)) for rv in rvs}
        for rv, val in observed.items():
            model.add_constraint(rv2var[rv] == bool(val))
        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = factor.input_rvs[0]
                model.add_constraint(rv2var[inp] == 1 - rv2var[rv])
            elif ftype == 'SAME':
                inp = factor.input_rvs[0]
                model.add_constraint(rv2var[inp] == rv2var[rv])
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                # Linearization of out = inp1 * inp2 for binary variables
                model.add_constraint(rv2var[rv] <= rv2var[inp1])
                model.add_constraint(rv2var[rv] <= rv2var[inp2])
                model.add_constraint(rv2var[rv] >= rv2var[inp1] + rv2var[inp2] - 1)

        model.maximize(list(rv2var.values())[0])
        model.print_information()
        sol = model.solve()
        print(model.solve_details)
        solution = {rv: sol[rv2var[rv]] for rv in rvs}
        return solution
