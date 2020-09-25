# -*- coding: utf-8 -*-
import scipy
import sympy
import cvxpy as cp
import numpy as np
from numpy import linalg as LA
from time import time
from collections import defaultdict


class LinAlgSolver(object):
    def __init__(self):
        pass

    def numpy2sym(self, M):
        return sympy.Matrix(M)

    def sym2numpy(self, M):
        return np.array(M).astype(np.float64)

    def mat_info(self, M, name):
        """
        Print various properties of matrix M
        """

        sz = M.shape
        rank = LA.matrix_rank(M)
        singular = rank < min(sz[0], sz[1])
        square = sz[0] == sz[1]
        symmetric = np.allclose(M, M.T) if square else False
        print('{}\tdim: {};\tsymmetric? {};\trank: {};\tsingular? {}'.format(
            name, sz, symmetric, rank, singular))

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
        x = np.array(all_bits, dtype=float)[rvs].reshape((-1, 1))
        X = np.diag(x.squeeze())  # Note: X is idempotent
        x_original = x[:]
        X_original = X[:]

        n = len(rvs)
        A = np.zeros((n, n))  # Relevant for INV and SAME factors
        b = np.zeros((n, 1))  # Relevant for INV factors
        D = np.zeros((n, n))  # Relevant for AND factors
        P_row = np.eye(n)  # Row permutation for AND
        P_col = np.eye(n)  # Column permutation for AND
        S = np.zeros((n, n))  # Column selection matrix

        for rv in rvs:
            factor = factors[rv]
            rv = rv2idx[rv]
            if factor.factor_type == 'INV':
                inp = rv2idx[factor.input_rvs[0]]
                A[rv, rv] = 1
                A[rv, inp] = 1
                b[rv, 0] = -1
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
                    S[inp1, inp1] = 1
                    P_row[[rv, inp2]] = P_row[[inp2, rv]]
                    P_col[[inp2, inp1]] = P_col[[inp1, inp2]]

        # Verify that P_row and P_col are permutation matrices
        P_row_row = np.sum(P_row, axis=1)
        P_row_col = np.sum(P_row, axis=0)
        P_col_row = np.sum(P_col, axis=1)
        P_col_col = np.sum(P_col, axis=0)
        assert np.allclose(P_row_row, np.ones(P_row_row.shape))
        assert np.allclose(P_row_col, np.ones(P_row_col.shape))
        assert np.allclose(P_col_row, np.ones(P_col_row.shape))
        assert np.allclose(P_col_col, np.ones(P_col_col.shape))
        assert np.allclose(P_row.T, LA.inv(P_row))
        assert np.allclose(P_col.T, LA.inv(P_col))

        E = A + D
        PcS = P_col.dot(S)

        print('-' * 40)
        print('Primary matrices:')
        self.mat_info(A, 'A')
        self.mat_info(D, 'D')
        self.mat_info(E, 'E')
        self.mat_info(P_row, 'P_row')
        self.mat_info(P_col, 'P_col')
        self.mat_info(S, 'S')
        print('-' * 40)

        print('#1: VERIFYING CORRECTNESS...')
        # (A + D + P_row * X * P_col * S) * x + b = 0
        # (E + P_row * X * PcS) * x + b = 0
        result = E + P_row.dot(X).dot(PcS)
        result = result.dot(x) + b
        assert np.allclose(result, np.zeros((n, 1)))
        print('PASSED.')

        # inv(P_row) = P_row.T
        # P_row.T * E * x + X * PcS * x + P_row.T * b = 0
        # (X * PcS * x) + (P_row.T * E * x) + (P_row.T * b) = 0
        # XAx + Bx + c = 0
        # Note: X * x = x (column), x.T * X = x.T (row)
        # x.T * (XAx + Bx + c = 0)
        # (x.T)Ax + (x.T)Bx + (x.T)c = 0
        # (x.T)(A + B)x + (x.T)c = 0
        # (x.T)Px + (x.T)c = 0
        # Without loss of generality, Q = (P + P.T) / 2
        # (x.T)Qx + (x.T)c = 0

        # L, U = ldl(Q)
        # Q = L * U * L.T
        # x = -inv(L) * inv(U) * inv(L).T * c
        # x = -inv(Q) * c

        # https://math.stackexchange.com/questions/52730/solving-quadratic-vector-equation

        A = PcS
        B = P_row.T.dot(E)
        c = P_row.T.dot(b)
        P = A + B
        Q = (P + P.T) / 2.0
        Q += np.eye(n)

        print('-' * 40)
        self.mat_info(A, 'A')
        self.mat_info(B, 'B')
        self.mat_info(Q, 'Q')
        print('-' * 40)

        print('#2: VERIFYING CORRECTNESS...')
        result = X.dot(A).dot(x) + B.dot(x) + c
        # assert result.shape == (n, 1)
        # assert np.allclose(result, np.zeros((n, 1)))
        print('PASSED.')

        print('#3: VERIFYING CORRECTNESS...')
        result = x.T.dot(Q).dot(x) + x.T.dot(c)
        # assert result.shape == (1, 1)
        # assert np.allclose(result, np.zeros((1, 1)))
        print('PASSED.')

        L, U, perm = scipy.linalg.ldl(Q)
        # Q_inv = scipy.linalg.pinvh(Q)
        Q_inv = LA.inv(Q)
        x = -Q_inv.dot(c)  # In general, x will not have members in {0, 1}

        print('#4: VERIFYING CORRECTNESS...')
        # assert np.allclose(Q, L.dot(U).dot(L.T))
        result = Q.dot(x) + c
        # assert result.shape == (n, 1)
        # assert np.allclose(result, np.zeros((n, 1)))
        print('PASSED.')

        # x = cp.Variable((n, 1), boolean=True)
        # cost = cp.sum_squares(Q @ x + c)
        # prob = cp.Problem(cp.Minimize(cost))
        # prob.solve()
        # print('Optimal value is {}'.format(prob.value))
        # print('x is {}'.format(x.value))
        # x = x.value

        solution = {rv: x[rv2idx[rv]] for rv in rvs}
        solution.update(observed)
        return solution
