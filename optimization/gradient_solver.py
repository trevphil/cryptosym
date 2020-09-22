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
        init_guess = np.ones(num_rvs) * 0.5
        for rv, val in observed.items():
            init_guess[rv2idx[rv]] = float(val)

        factors_per_rv = {rv: set([rv]) for rv in rv_indices}
        for factor_idx, factor in factors.items():
            for ref in factor.referenced_rvs:
                factors_per_rv[ref].add(factor_idx)

        # Ax = b (for INV) --> Ax - b = 0
        A = np.zeros((num_rvs, num_rvs))
        b = np.zeros((num_rvs, 1))

        # A_obs * x = b_obs
        A_obs = np.zeros((num_rvs, num_rvs))
        b_obs = np.zeros((num_rvs, 1))
        for rv, val in observed.items():
            i = rv2idx[rv]
            A_obs[i, i] = 1.0
            b_obs[i] = float(val)

        for rv in rv_indices:
            factor = factors[rv]
            if factor.factor_type == 'INV':
                inp, out = factor.input_rvs[0], factor.output_rv
                inp, out = rv2idx[inp], rv2idx[out]
                # x_i + x_j = 1.0 since the RVs are inverses
                A[out, out] = 1.0
                A[out, inp] = 1.0
                b[out] = 1.0

        and_factor_indices = [rv for rv in rv_indices
                            if factors[rv].factor_type == 'AND']

        def f(x):
            for i in and_factor_indices:
                factor = factors[i]
                inp1, inp2 = factor.input_rvs
                out = factor.output_rv
                inp1, inp2, out = rv2idx[inp1], rv2idx[inp2], rv2idx[out]
                # Update the A matrix s.t. inp1 * inp2 - out = 0.0
                A[out, out] = -1.0
                A[out, inp1] = x[inp2]

            x = x.reshape((-1, 1))
            err_sq = (A @ x - b) ** 2
            obs_err_sq = (A_obs @ x - b_obs) ** 2
            return np.sum(err_sq) + np.sum(obs_err_sq)

        # Jacobian: J[i] = derivative of f(x) w.r.t. variable `i`
        def jacobian(x):
            J = np.zeros(num_rvs)
            for rv in rv_indices:
                for factor_idx in factors_per_rv[rv]:
                    i = rv2idx[rv]
                    J[i] += factors[factor_idx].first_order(rv, x, rv2idx)
            for rv, val in observed.items():
                i = rv2idx[rv]
                J[i] += 2 * (x[i] - float(val))
            return J

        # Hessian: H[i, j] = derivative of J[j] w.r.t. variable `i`
        def hessian(x):
            H = np.zeros((num_rvs, num_rvs))
            for rv_j in rv_indices:
                for factor_idx in factors_per_rv[rv_j]:
                    factor = factors[factor_idx]
                    for rv_i in factor.referenced_rvs:
                        i, j = rv2idx[rv_i], rv2idx[rv_j]
                        H[i, j] += factor.second_order(rv_j, rv_i, x, rv2idx)
            for rv, val in observed.items():
                i = rv2idx[rv]
                H[i, i] += 2.0
            assert np.sum(np.abs(H - H.T)) < 1e-5, 'Hessian is not symmetric'
            return H

        method = 'trust-ncg'
        options = {'maxiter': 400, 'disp': False}

        start = time()
        print('Starting optimization...')
        result = minimize(f, init_guess, method=method, options=options,
                        jac=jacobian, hess=hessian)
        print('Optimization finished in %.2f s' % (time() - start))
        # print('\tsuccess: {}'.format(result['success']))
        print('\tstatus:  {}'.format(result['message']))
        print('\terror:   {:.2f}'.format(result['fun']))

        pred = lambda rv: result['x'][rv2idx[rv]]
        return {rv: (1.0 if pred(rv) > 0.5 else 0.0) for rv in rv_indices}
