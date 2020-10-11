# -*- coding: utf-8 -*-

import scipy
import sympy
import numpy as np
from scipy import sparse
from scipy.sparse import linalg as LA
from collections import defaultdict
from matplotlib import pyplot as plt
# from qpsolvers import solve_qp

from optimization.utils import save_to_matlab_matrix

class LinAlgSolver(object):
    def __init__(self):
        self.check = True
        self.visualize = False
        self.should_save_to_matlab = False
        self.dtype = np.int32

    def numpy2sym(self, M):
        return sympy.Matrix(M)

    def sym2numpy(self, M):
        return np.array(M).astype(self.dtype)

    def simplify(self, M):
        if type(M) is np.ndarray:
            return M
        M.sum_duplicates()
        M.eliminate_zeros()
        return M

    def allones(self, M):
        return (M == 1).all()

    def allzeros(self, M):
        return (M == 0).all()

    def allclose(self, a, b, rtol=1e-5, atol = 1e-8):
        return self.simplify(a - b).nnz == 0

    def swap_rows(self, M, row1, row2):
        M[[row1, row2]] = M[[row2, row1]]

    def mat_info(self, M, name):
        """
        Print various properties of matrix M
        """

        sz = M.shape
        rank = np.linalg.matrix_rank(M.todense())
        singular = rank < min(sz[0], sz[1])
        square = sz[0] == sz[1]
        symmetric = self.allclose(M, M.T) if square else False
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

    def solve(self, factors, observed, config, all_bits):
        solvable = self.verify_solvable(factors)
        assert solvable, 'Some RVs are inputs to more than one AND-gate!'

        obs_rv_set = set(observed.keys())
        rvs = list(sorted(factors.keys()))
        rv2idx = {rv: i for i, rv in enumerate(rvs)}
        n = len(rvs)

        # X is just to verify the linear algebra works correctly.
        # In reality we would not have access to all of the bits.
        x = np.array(all_bits, dtype=self.dtype)[rvs].reshape((-1, 1))
        X = sparse.coo_matrix((n, n), dtype=self.dtype)
        X.setdiag(x.squeeze())  # Note: X is idempotent
        x_original = x[:]
        X_original = X.copy()

        b = np.zeros((n, 1))  # Relevant for INV and observed factors
        Ar, Ac, Ad = [], [], []  # Relevant for INV and SAME factors
        Dr, Dc, Dd = [], [], []  # Relevant for AND factors
        P_row = sparse.identity(n, format='lil', dtype=self.dtype)  # Row permutation for AND
        P_col = sparse.identity(n, format='lil', dtype=self.dtype)  # Column permutation for AND
        Sr, Sc, Sd = [], [], []  # Column selection matrix

        for rv in rvs:
            factor = factors[rv]
            rv = rv2idx[rv]
            if factor.factor_type == 'INV':
                inp = rv2idx[factor.input_rvs[0]]
                Ar.append(rv); Ac.append(rv); Ad.append(1)
                Ar.append(rv); Ac.append(inp); Ad.append(1)
                b[rv, 0] = -1
            elif factor.factor_type == 'SAME':
                inp = rv2idx[factor.input_rvs[0]]
                Ar.append(rv); Ac.append(rv); Ad.append(-1)
                Ar.append(rv); Ac.append(inp); Ad.append(1)
            elif factor.factor_type == 'AND':
                inp1, inp2 = factor.input_rvs[:2]
                out_is_observed = (factor.output_rv in obs_rv_set)
                inp1_is_observed = (inp1 in obs_rv_set)
                inp2_is_observed = (inp2 in obs_rv_set)
                # Special case if one of the inputs is observed and equal to 1.
                # In this case, the other two bits must be the SAME.
                # There should never be both inputs observed and output unobserved.
                assert not (inp1_is_observed and inp2_is_observed and not out_is_observed)
                if inp1_is_observed and observed[inp1] == True:
                    Ar.append(rv); Ac.append(rv); Ad.append(-1)
                    Ar.append(rv); Ac.append(rv2idx[inp2]); Ad.append(1)
                elif inp2_is_observed and observed[inp2] == True:
                    Ar.append(rv); Ac.append(rv); Ad.append(-1)
                    Ar.append(rv); Ac.append(rv2idx[inp1]); Ad.append(1)
                else:
                    # An AND-gate input can only participate in one AND-gate!!!
                    # Otherwise the row and column permutations are invalid.
                    inp1, inp2 = rv2idx[inp1], rv2idx[inp2]
                    Dr.append(rv); Dc.append(rv); Dd.append(-1)
                    Sr.append(inp1); Sc.append(inp1); Sd.append(1)
                    self.swap_rows(P_row, rv, inp2)
                    self.swap_rows(P_col, inp2, inp1)

        A = sparse.coo_matrix((Ad, (Ar, Ac)), shape=(n, n), dtype=self.dtype).tocsr()
        D = sparse.coo_matrix((Dd, (Dr, Dc)), shape=(n, n), dtype=self.dtype).tocsr()
        S = sparse.coo_matrix((Sd, (Sr, Sc)), shape=(n, n), dtype=self.dtype).tocsr()
        P_row = P_row.tocsr()
        P_col = P_col.tocsr()
        Ad = Ar = Ac = Dd = Dr = Dc = Sd = Sr = Sc = None

        if self.check:
            # Verify that P_row and P_col are permutation matrices
            P_row_row = np.sum(P_row, axis=1)
            P_row_col = np.sum(P_row, axis=0)
            P_col_row = np.sum(P_col, axis=1)
            P_col_col = np.sum(P_col, axis=0)
            assert self.allones(P_row_row)
            assert self.allones(P_row_col)
            assert self.allones(P_col_row)
            assert self.allones(P_col_col)
            assert self.allclose(P_row.T, LA.inv(P_row))
            assert self.allclose(P_col.T, LA.inv(P_col))

        E = A + D
        PcS = P_col.dot(S)

        if self.check:
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
            result = self.simplify(E + P_row.dot(X).dot(PcS))
            result = result.dot(x) + b
            assert result.shape == (n, 1)
            assert self.allzeros(result)
            print('PASSED.')

        # inv(P_row) = P_row.T
        # P_row.T * E * x + X * PcS * x + b = 0
        # (X * PcS * x) + (P_row.T * E * x) + (P_row.T * b) = 0
        # Note: X * x = x (column), x.T * X = x.T (row)
        # (x.T)(PcS)x + (x.T)(P_row.T * E)x + (x.T)(P_row.T * b) = 0
        # (x.T)(PcS + P_row.T * E)x + (x.T)(P_row.T * b) = 0
        # P = PcS + P_row.T * E
        # Q = P + P.T
        # c = 2 * P_row.T * b
        # (x.T)Qx + (x.T)c = 0
        # Take the transpose of everything to get QP format...
        # (x.T)((x.T)Q).T + (c.T)x = 0
        # (x.T)Qx + (c.T)x = 0

        P = PcS + P_row.T.dot(E)
        Q = P + P.T
        Qd = Q.todense()
        c = 2 * P_row.T.dot(b)

        if self.check:
            print('-' * 40)
            self.mat_info(Q, 'Q')
            print('-' * 40)

            print('#2: VERIFYING CORRECTNESS...')
            result = x.T.dot(Qd).dot(x) + x.T.dot(c)
            assert result == 0
            print('PASSED.')

        if self.should_save_to_matlab:
            save_to_matlab_matrix(dict(Q=Q), 'Q.mat')

        if self.visualize:
            mat = Q if n <= 400 else Q[:400, :400]
            mat = mat.todense().astype(float)
            pos = plt.imshow(mat, cmap='jet', interpolation='nearest')
            plt.colorbar(pos)
            plt.show()

        # Constraints that each RV is 0 <= RV <= 1
        # Gx <= h   --->   1x <= 1,  -1x <= -0
        G = np.vstack((np.eye(n), -np.eye(n))).astype(np.double)
        h = np.vstack((np.ones((n, 1)), np.zeros((n, 1)))).astype(np.double)

        # Equality constraints on observed RVs
        # Ax = b
        A = np.zeros(G.shape, dtype=np.double)
        b = np.zeros(h.shape, dtype=np.double)
        for obs_rv, obs_val in observed.items():
            obs_rv = rv2idx[obs_rv]
            A[obs_rv, obs_rv] = 1
            b[obs_rv, 0] = float(obs_val)

        # save_to_matlab_matrix(dict(Q=Q, c=c, G=G, h=h, A=A, b=b), 'data.mat')

        solution = {rv: x[rv2idx[rv]] for rv in rvs}
        solution.update(observed)
        return solution
