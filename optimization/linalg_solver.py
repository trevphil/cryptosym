# -*- coding: utf-8 -*-
import scipy
import numpy as np
from numpy import linalg as LA
from time import time
from collections import defaultdict


class LinAlgSolver(object):
    def __init__(self):
        pass

    def mat_info(self, M, name):
        """
        Print various properties of matrix M
        """

        sz = M.shape
        rank = LA.matrix_rank(M)
        square = sz[0] == sz[1]
        symmetric = np.allclose(M, M.T) if square else False
        det = np.round(LA.det(M), 3) if square else 'N/A'
        invertible = det != 0 if square else False
        print('{}\tdim: {};\tsymmetric? {};\trank: {};\tdet: {};\tinvertible? {}'.format(
            name, sz, symmetric, rank, det, invertible))

    def verify_solvable(self, factors):
        and_gates_per_rv = defaultdict(lambda: 0)
        for rv, factor in factors.items():
            if factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                and_gates_per_rv[inp1] += 1
                and_gates_per_rv[inp2] += 1
                if and_gates_per_rv[inp1] > 1:
                    return False
                if and_gates_per_rv[inp2] > 1:
                    return False
        return True

    def set_implicit_observed(self, factors, observed, all_bits):
        """
        Some bits can be figured out, e.g. if bit X = INV(bit Y) and
        bit X is observed as 0, then we can infer that bit Y = 1.
        If bit X = 1 = AND(bit Y, bit Z) then bits Y and Z are both 1.
        """

        initial = len(observed)
        print('There are %d initially observed bits.' % initial)

        def backward(observed):
            queue = list(observed.keys())
            smallest_obs = len(all_bits)

            while len(queue) > 0:
                rv = queue.pop(0)
                smallest_obs = min(smallest_obs, rv)
                observed[rv] = bool(all_bits[rv])
                factor = factors[rv]
                if factor.factor_type in ('INV', 'SAME'):
                    inp = factor.input_rvs[0]
                    queue.append(inp)
                elif factor.factor_type == 'AND' and observed[rv]:
                    inp1, inp2 = factor.input_rvs[:2]
                    queue.append(inp1)
                    queue.append(inp2)

            return observed, smallest_obs

        def forward(observed, smallest_obs):
            obs_rv_set = set(observed.keys())
            for rv in range(smallest_obs, len(all_bits)):
                factor = factors.get(rv, None)
                if factor is None:
                    continue
                if factor.factor_type in ('INV', 'SAME'):
                    inp = factor.input_rvs[0]
                    if inp in obs_rv_set:
                        observed[rv] = bool(all_bits[rv])
                elif factor.factor_type == 'AND':
                    inp1, inp2 = factor.input_rvs[:2]
                    inp1_is_observed = (inp1 in obs_rv_set)
                    inp2_is_observed = (inp2 in obs_rv_set)
                    if inp1_is_observed and inp2_is_observed:
                        observed[rv] = bool(all_bits[rv])
                    elif inp1_is_observed and observed[inp1] == False:
                        observed[rv] = bool(all_bits[rv])
                    elif inp2_is_observed and observed[inp2] == False:
                        observed[rv] = bool(all_bits[rv])
            return observed

        while True:
            before = len(observed)
            observed, smallest_obs = backward(observed)
            observed = forward(observed, smallest_obs)
            after = len(observed)
            if before == after:
                break  # No new observed variables were derived

        print('Found %d additional observed bits.' % (len(observed) - initial))
        return observed

    def solve(self, factors, observed, config, all_bits):
        solvable = self.verify_solvable(factors)
        assert solvable, 'Some RVs are inputs to more than one AND-gate!'

        observed = self.set_implicit_observed(factors, observed, all_bits)
        if len(observed) == len(factors):
            return observed  # Everything was solved already :)

        obs_rv_set = set(observed.keys())
        rvs = set(factors.keys()).difference(obs_rv_set)
        rvs = list(sorted(rvs))
        rv2idx = {rv: i for i, rv in enumerate(rvs)}

        # X is just to verify the linear algebra works correctly.
        # In reality we would not have access to all of the bits.
        x = np.array(all_bits, dtype=float)[rvs]
        X = np.diag(x)

        n = len(rvs)
        A = np.zeros((n, n))  # Relevant for INV and SAME factors
        B = np.zeros((n, n))  # Relevant for INV factors
        D = np.zeros((n, n))  # Relevant for AND factors
        P_row = np.zeros((n, n))  # Row (pseudo-)permutation for AND
        P_col = np.zeros((n, n))  # Column (pseudo-)permutation for AND

        for rv in rvs:
            factor = factors[rv]
            rv = rv2idx[rv]
            if factor.factor_type == 'INV':
                inp = rv2idx[factor.input_rvs[0]]
                A[rv, rv] = 1
                A[rv, inp] = 1
                B[rv, rv] = -1
            elif factor.factor_type == 'SAME':
                inp = rv2idx[factor.input_rvs[0]]
                A[rv, rv] = -1
                A[rv, inp] = 1
            elif factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                inp1_is_observed = (inp1 in obs_rv_set)
                inp2_is_observed = (inp2 in obs_rv_set)
                # Special case if one of the inputs is observed. In this case,
                # it must be equal to 1 and the other two bits must be the SAME.
                # There should never be both inputs observed and output unobserved.
                assert not (inp1_is_observed and inp2_is_observed)
                if inp1_is_observed:
                    assert observed[inp1] == True
                    A[rv, rv] = -1
                    A[rv, rv2idx[inp2]] = 1
                elif inp2_is_observed:
                    assert observed[inp2] == True
                    A[rv, rv] = -1
                    A[rv, rv2idx[inp1]] = 1
                else:
                    # An AND-gate input can only participate in one AND-gate!!!
                    # Otherwise the row and column permutations are invalid.
                    inp1, inp2 = rv2idx[inp1], rv2idx[inp2]
                    D[rv, rv] = -1
                    P_row[rv, inp2] = 1
                    P_col[inp2, inp1] = 1

        print('-' * 40)
        print('Primary matrices:')
        self.mat_info(A, 'A')
        self.mat_info(B, 'B')
        self.mat_info(D, 'D')
        self.mat_info(A + D, 'A+D')
        self.mat_info(P_row, 'P_row')
        self.mat_info(P_col, 'P_col')
        print('-' * 40)

        # (A + D + P_row * X * P_col) * X + B = 0
        # (E + P_r * X * P_c) * X + B = 0 where E = A + D
        # (inv(P_r) * E * X) + (X * P_c * X) + (inv(P_r) * B) = 0
        # XAX + BX + C = 0 where A = P_c; B = inv(P_r) * E; C = inv(P_r) * B

        print('#1: VERIFYING CORRECTNESS...')
        result = A + D
        result += np.matmul(P_row, np.matmul(X, P_col))
        result = np.matmul(result, X) + B
        result = np.sum(result, axis=1)
        assert np.allclose(result, np.zeros(result.shape))
        print('PASSED.')

        P_row_inv = LA.pinv(P_row)
        A_hat = P_col.T
        B_hat = np.matmul(P_row_inv, A + D).T
        # C_hat = np.matmul(P_row_inv, B).T
        C_hat = np.matmul(P_row_inv, B).T
        print('-' * 40)
        print('Info for matrix coefficients in the quadratic')
        self.mat_info(A_hat, 'A_hat')
        self.mat_info(B_hat, 'B_hat')
        self.mat_info(C_hat, 'C_hat')
        print('-' * 40)

        # (X - H_hat).T * A_hat * (X - H_hat) + K_hat = 0
        H_hat = np.matmul(-LA.pinv(A_hat + A_hat.T), B_hat)
        K_hat = C_hat - np.matmul(H_hat.T, np.matmul(A_hat, H_hat))
        print('-' * 40)
        print('Info for factorized coefficients')
        self.mat_info(H_hat, 'H_hat')
        self.mat_info(K_hat, 'K_hat')
        KA = np.matmul(K_hat, A_hat)
        AK = np.matmul(A_hat, K_hat)
        commute = np.round(np.sum(np.abs(KA - AK)), 2) == 0.0
        print('A_hat and K_hat commute? {}'.format(commute))
        print('-' * 40)

        prod = np.matmul(-K_hat, LA.pinv(A_hat))
        print('-' * 40)
        self.mat_info(prod, '-K_hat * pinv(A_hat)')
        print('-' * 40)
        # sq_root = scipy.linalg.sqrtm(prod)
        U, Q = scipy.linalg.schur(prod, output='real')
        print('Computed schur')
        sqrt_U = np.asmatrix(scipy.linalg.sqrtm(U))
        sq_root = np.matmul(Q, np.matmul(sqrt_U, LA.inv(Q)))
        X = H_hat + sq_root
        x = np.diag(X)

        solution = dict()
        for rv in rvs:
            if rv in obs_rv_set:
                solution[rv] = observed[rv]
            else:
                solution[rv] = x[rv2idx[rv]]
        return solution
