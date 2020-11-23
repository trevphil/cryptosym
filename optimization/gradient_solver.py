# -*- coding: utf-8 -*-
import numpy as np
from time import time
from scipy.optimize import minimize


class GradientSolver(object):
    def __init__(self):
        pass

    def solve(self, factors, observed, config, all_bits):
        rv_indices = list(sorted(rv for rv in factors.keys()))
        rv2idx = {rv_index: i for i, rv_index in enumerate(rv_indices)}
        num_rvs = len(rv_indices)

        A_eq = np.zeros((0, num_rvs))  # Ax + b = 0
        b_eq = np.zeros((0, ))
        A_ineq = np.zeros((0, num_rvs))  # Ax + b >= 0
        b_ineq = np.zeros((0, ))
        for rv, factor in factors.items():
            rv = rv2idx[rv]
            if factor.factor_type in ('INV', 'SAME'):
                row = np.zeros(num_rvs)
                row[rv] = 1
                inp = rv2idx[factor.input_rvs[0]]
                if factor.factor_type == 'INV':
                    row[inp] = 1  # X = 1 - Y
                    b_eq = np.concatenate((b_eq, (-1, )))
                elif factor.factor_type == 'SAME':
                    row[inp] = -1  # X = Y
                    b_eq = np.concatenate((b_eq, (0, )))
                A_eq = np.vstack((A_eq, row))
            elif factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs
                inp1, inp2 = rv2idx[inp1], rv2idx[inp2]
                row1 = np.zeros(num_rvs)
                row1[rv] = -1
                row1[inp1] = 1
                b1 = (0, )  # X <= Y
                row2 = np.zeros(num_rvs)
                row2[rv] = -1
                row2[inp2] = 1
                b2 = (0, )  # X <= Z
                row3 = np.zeros(num_rvs)
                row3[rv] = 1
                row3[inp1] = -1
                row3[inp2] = -1
                b3 = (-1, )  # X >= Y + Z - 1
                A_ineq = np.vstack((A_ineq, row1, row2, row3))
                b_ineq = np.concatenate((b_ineq, b1, b2, b3))

        # x >= 0
        A_ineq = np.vstack((A_ineq, np.eye(num_rvs)))
        b_ineq = np.concatenate((b_ineq, np.zeros(num_rvs)))

        init_guess = np.ones(num_rvs) * 0.5
        c = np.zeros(num_rvs)  # x = c
        E = np.zeros((num_rvs, num_rvs))
        for rv, val in observed.items():
            rv = rv2idx[rv]
            init_guess[rv] = float(val)
            c[rv] = float(val)
            E[rv, rv] = 1

        def eq_cons_fn(x):
            return A_eq @ x + b_eq

        def eq_cons_jac(x):
            return A_eq

        def ineq_cons_fn(x):
            return A_ineq @ x + b_ineq

        def ineq_cons_jac(x):
            return A_ineq

        def f(x):
            y = x - c
            feval = y.T @ E @ y
            jac = 2 * E @ y
            return feval, jac

        cons = (
            {'type': 'eq', 'fun': eq_cons_fn, 'jac': eq_cons_jac},
            {'type': 'ineq', 'fun': ineq_cons_fn, 'jac': ineq_cons_jac}
        )

        method = 'SLSQP'
        options = {'maxiter': 400, 'disp': False}

        start = time()
        print('Starting optimization...')
        result = minimize(f, init_guess, method=method, constraints=cons,
                          jac=True, options=options)
        print('Optimization finished in %.2f s' % (time() - start))
        print(result)

        pred = lambda rv: result.x[rv2idx[rv]]
        return {rv: (1.0 if pred(rv) > 0.5 else 0.0) for rv in rv_indices}
