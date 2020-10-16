# -*- coding: utf-8 -*-

from multiprocessing import cpu_count

from docplex import cp
from docplex.cp.model import CpoModel, CpoParameters


class CplexCPSolver(object):
    def solve(self, factors, observed, config, all_bits):
        rvs = list(sorted(factors.keys()))

        # https://cdn.rawgit.com/IBMDecisionOptimization/docplex-doc/master/docs/cp/creating_model.html
        model = CpoModel()
        # model.context.cplex_parameters.threads = cpu_count()
        rv2var = {rv: model.binary_var(str(rv)) for rv in rvs}
        for rv, val in observed.items():
            model.add(rv2var[rv] == bool(val))
        for rv, factor in factors.items():
            ftype = factor.factor_type
            if ftype == 'INV':
                inp = factor.input_rvs[0]
                model.add(rv2var[inp] == 1 - rv2var[rv])
            elif ftype == 'SAME':
                inp = factor.input_rvs[0]
                model.add(rv2var[inp] == rv2var[rv])
            elif ftype == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                # Linearization of out = inp1 * inp2 for binary variables
                model.add(rv2var[rv] <= rv2var[inp1])
                model.add(rv2var[rv] <= rv2var[inp2])
                model.add(rv2var[rv] >= rv2var[inp1] + rv2var[inp2] - 1)

        # https://cdn.rawgit.com/IBMDecisionOptimization/docplex-doc/master/docs/cp/docplex.cp.parameters.py.html#docplex.cp.parameters.CpoParameters
        params = CpoParameters(Workers=cpu_count())

        model.print_information()
        sol = model.solve(agent='local', params=params,
            execfile='/Applications/CPLEX_Studio1210/cpoptimizer/bin/x86-64_osx/cpoptimizer')
        solution = {rv: sol.get_value(rv2var[rv]) for rv in rvs}
        return solution
