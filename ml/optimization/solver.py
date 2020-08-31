# -*- coding: utf-8 -*-
import numpy as np
from time import time
from scipy.optimize import minimize

from optimization.gnc import GNC


def solve(factors, observed, config):
    n = int(config['num_rvs'])
    init_guess = np.ones(n) * 0.5
    for rv, val in observed.items():
        init_guess[rv] = float(val)

    factors_per_rv = [set([i]) for i in range(n)]
    for factor_idx, factor in factors.items():
        for rv in factor.referenced_rvs:
            factors_per_rv[rv].add(factor_idx)

    # Ax = b (for SAME and INV) --> Ax - b = 0
    A = np.zeros((n, n))
    b = np.zeros((n, 1))

    # A_obs * x = b_obs
    A_obs = np.zeros((n, n))
    b_obs = np.zeros((n, 1))
    for rv, val in observed.items():
      A_obs[rv, rv] = 1.0
      b_obs[rv] = float(val)

    for i in range(n):
        factor = factors[i]
        if factor.factor_type == 'SAME':
            inp, out = factor.input_rvs[0], factor.output_rv
            # x_i - x_j = 0.0 since the RVs are the same
            A[out, out] = 1.0
            A[out, inp] = -1.0
        elif factor.factor_type == 'INV':
            inp, out = factor.input_rvs[0], factor.output_rv
            # x_i + x_j = 1.0 since the RVs are inverses
            A[out, out] = 1.0
            A[out, inp] = 1.0
            b[out] = 1.0

    and_factor_indices = [i for i in range(n)
                          if factors[i].factor_type == 'AND']

    C = 0.02
    C_sq = C * C
    gnc = GNC(C)

    def cb(x, foo):
        gnc.increment()

    def f(x):
        for i in and_factor_indices:
            factor = factors[i]
            inp1, inp2 = factor.input_rvs
            out = factor.output_rv
            # Update the A matrix s.t. inp1 * inp2 - out = 0.0
            A[out, out] = -1.0
            A[out, inp1] = x[inp2]

        x = x.reshape((-1, 1))
        err_sq = (A @ x - b) ** 2
        obs_err_sq = (A_obs @ x - b_obs) ** 2
        return np.sum(err_sq) + np.sum(obs_err_sq)
        # mu = gnc.mu(err_sq)
        # err = (err_sq * C_sq * mu) / (err_sq + mu * C_sq)
        # return np.sum(err)

    # Jacobian: J[i] = derivative of f(x) w.r.t. variable `i`
    def jacobian(x):
        J = np.zeros(n)
        for i in range(n):
            for factor_idx in factors_per_rv[i]:
                J[i] += factors[factor_idx].first_order(i, x)
        for i, val in observed.items():
            J[i] += 2 * (x[i] - float(val))
        return J

    # Hessian: H[i, j] = derivative of J[j] w.r.t. variable `i`
    def hessian(x):
        H = np.zeros((n, n))
        for j in range(n):
            for factor_idx in factors_per_rv[j]:
                factor = factors[factor_idx]
                for i in factor.referenced_rvs:
                    H[i, j] += factor.second_order(j, i, x)
        for i, val in observed.items():
            H[i, i] += 2.0
        assert np.sum(np.abs(H - H.T)) < 1e-5, 'Hessian is not symmetric'
        return H

    method = 'trust-constr'
    options = {'maxiter': 40, 'disp': False}

    start = time()
    print('Starting optimization...')
    result = minimize(f, init_guess, method=method, options=options,
                      jac=jacobian, hess=hessian, callback=cb)
    print('Optimization finished in %.2f s' % (time() - start))
    # print('\tsuccess: {}'.format(result['success']))
    print('\tstatus:  {}'.format(result['message']))
    print('\terror:   {:.2f}'.format(result['fun']))

    return [(1.0 if rv > 0.5 else 0.0) for rv in result['x']]
