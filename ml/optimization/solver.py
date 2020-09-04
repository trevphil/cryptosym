# -*- coding: utf-8 -*-
import numpy as np
from time import time
from scipy.optimize import minimize

from optimization.gnc import GNC

class Solver(object):
    def __init__(self):
        # C should be set to be the maximum error expected for inliers
        self.C = 0.001
        self.C_sq = self.C * self.C
        self.sq_residuals = np.array([])
        self.weights = dict()
        self.gnc = GNC(self.C)

    def solve(self, factors, observed, config):
        rv_indices = []
        num_and, num_priors = 0, 0
        for rv, factor in factors.items():
            rv_indices.append(rv)
            if factor.factor_type == 'AND':
                num_and += 1
            elif factor.factor_type == 'PRIOR':
                num_priors += 1
        rv_indices = list(sorted(rv_indices))
        rv2idx = {rv_index: i for i, rv_index in enumerate(rv_indices)}

        num_rvs = len(rv_indices)
        num_residuals = (num_rvs - num_and) + 2 * (num_and * 4) \
                        + len(observed) - num_priors
        self.sq_residuals = np.zeros(num_residuals)

        init_guess = np.ones(num_rvs) * 0.5
        for rv, val in observed.items():
            init_guess[rv2idx[rv]] = float(val)

        def cb(x, foo=None):
            mu = self.gnc.mu(self.sq_residuals)
            for r_idx, _ in self.weights.items():
                r_sq = self.sq_residuals[r_idx]
                w_update = (mu * self.C_sq / (r_sq + mu * self.C_sq)) ** 2
                self.weights[r_idx] = w_update
            self.gnc.increment()

        def f(x):
            r_idx = 0
            self.sq_residuals = np.zeros(self.sq_residuals.shape)
            for rv in rv_indices:
                factor = factors[rv]
                ftype = factor.factor_type
                i = rv2idx[rv]
                obs_val = observed.get(rv, None)

                if obs_val is not None:
                    self.sq_residuals[r_idx] = (x[i] - float(obs_val)) ** 2
                    r_idx += 1
                if ftype == 'SAME':
                    inp = rv2idx[factor.input_rvs[0]]
                    self.sq_residuals[r_idx] = (x[i] - x[inp]) ** 2
                    r_idx += 1
                elif ftype == 'INV':
                    inp = rv2idx[factor.input_rvs[0]]
                    self.sq_residuals[r_idx] = (1.0 - x[i] - x[inp]) ** 2
                    r_idx += 1
                elif ftype == 'AND':
                    inp1, inp2 = factor.input_rvs
                    inp1, inp2 = x[rv2idx[inp1]], x[rv2idx[inp2]]
                    a = inp1 ** 2 + inp2 ** 2 + x[i] ** 2  # AND(0, 0) = 0
                    b = inp1 ** 2 + (1-inp2) ** 2 + x[i] ** 2  # AND(0, 1) = 0
                    c = (1-inp1) ** 2 + inp2 ** 2 + x[i] ** 2  # AND(1, 0) = 0
                    d = (1-inp1) ** 2 + (1-inp2) ** 2 + (1-x[i]) ** 2  # AND(1, 1) = 1

                    # Black-Rangarajan duality of Geman-McClure robust cost function
                    penalties = []
                    for residual in [a, b, c, d]:
                        w = self.weights.get(r_idx, 1.0)
                        self.sq_residuals[r_idx] = residual * residual
                        penalty = self.C_sq * ((np.sqrt(w) - 1) ** 2)  # times mu (later)
                        penalties.append(penalty)
                        self.weights[r_idx] = w
                        r_idx += 1

                    mu = self.gnc.mu(self.sq_residuals)
                    for penalty in penalties:
                        self.sq_residuals[r_idx] = mu * penalty
                        r_idx += 1

                elif ftype != 'PRIOR':
                    raise NotImplementedError('Unknown factor: %s' % ftype)

            assert r_idx == self.sq_residuals.shape[0], 'Index misalignment'

            sq_res = self.sq_residuals[:]
            for r_idx, weight in self.weights.items():
                sq_res[r_idx] *= weight
            return np.sum(sq_res)

        method = 'Nelder-Mead'
        options = {'adaptive': True, 'disp': False, 'maxiter': 100}
        # method = 'CG'
        # options = {'eps': 1e-6}

        start = time()
        print('Starting optimization...')
        result = minimize(f, init_guess, callback=cb, tol=1e-6,
                          method=method, options=options)
        print('Optimization finished in %.2f s' % (time() - start))
        print('\tsuccess: {}'.format(result['success']))
        print('\tstatus:  {}'.format(result['message']))
        print('\terror:   {:.2f}'.format(result['fun']))

        pred = lambda rv: result['x'][rv2idx[rv]]
        return {rv: (1.0 if pred(rv) > 0.5 else 0.0) for rv in rv_indices}
