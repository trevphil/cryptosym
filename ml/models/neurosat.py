import torch

from models.base_model import BaseModel
from models.mlp import MLP
from models.layer_norm_lstm import LayerNormLSTMCell


class NeuroSAT(BaseModel):
    def __init__(self, opts, problem, layer_norm=True):
        super().__init__(opts, problem)
        # LSTM
        if layer_norm:
            self.L_lstm = LayerNormLSTMCell(input_size=opts.d,
                                            hidden_size=opts.d)
            self.C_lstm = LayerNormLSTMCell(input_size=opts.d,
                                            hidden_size=opts.d)
        else:
            self.L_lstm = torch.nn.LSTMCell(input_size=opts.d,
                                            hidden_size=opts.d,
                                            bias=True)
            self.C_lstm = torch.nn.LSTMCell(input_size=opts.d,
                                            hidden_size=opts.d,
                                            bias=True)
        # Multi-layer perceptrons
        dims_out_msg = [opts.d for _ in range(opts.n_msg_layers)] + [opts.d]
        dims_out_vote = [opts.d for _ in range(opts.n_vote_layers)] + [1]
        self.L_msg = MLP(dim_in=opts.d, dims_out=dims_out_msg)
        self.C_msg = MLP(dim_in=opts.d, dims_out=dims_out_msg)
        self.L_vote = MLP(dim_in=opts.d, dims_out=dims_out_vote)

        self.norm = 1.0 / (opts.d ** 0.5)
        L_init = torch.normal(0, 1, size=(problem.num_vars, opts.d), dtype=opts.dtype) * self.norm
        C_init = torch.normal(0, 1, size=(problem.num_clauses, opts.d), dtype=opts.dtype) * self.norm
        self.L_init = torch.nn.parameter.Parameter(L_init, requires_grad=True)
        self.C_init = torch.nn.parameter.Parameter(C_init, requires_grad=True)

        # Sigmoid
        self.sigmoid = torch.nn.Sigmoid()

        print(f'{self.name} has {self.num_parameters} parameters.')

    @property
    def name(self):
        return 'NeuroSAT'

    def forward(self, observed):
        A = self.problem.A
        A_T = self.problem.A_T

        dtype = self.opts.dtype
        n_vars, n_clauses = A.size()[:2]  # n, m

        # Message tensors
        L_state = self.L_init.clone()
        C_state = self.C_init.clone()

        for var, val in observed.items():
            assert var > 0
            val = (1.0 if val else -1.0)
            L_state[var - 1, :] = val

        # Hidden states
        L_hidden = torch.zeros(n_vars, self.opts.d, dtype=dtype)     # n, d
        C_hidden = torch.zeros(n_clauses, self.opts.d, dtype=dtype)  # m, d

        L_state_prev = L_state
        converged = False
        itr = 0

        while not converged and itr < self.opts.T:
            L_msg_pre = self.L_msg(L_state).squeeze()  # n, d
            L_msg = torch.matmul(A_T, L_msg_pre)  # m, d

            C_hidden, C_state = self.C_lstm(L_msg, (C_hidden, C_state))

            C_msg_pre = self.C_msg(C_state).squeeze()  # m, d
            C_msg = torch.matmul(A, C_msg_pre)  # n, d

            L_hidden, L_state = self.L_lstm(C_msg, (L_hidden, L_state))

            change = (L_state - L_state_prev).norm(dim=1)
            L_state_prev = L_state
            converged = (torch.max(change) < 0.5)
            itr += 1

        votes = self.L_vote(L_state).squeeze()  # n
        assignments = self.sigmoid(votes)
        return assignments


if __name__ == '__main__':
    print('Testing the model\'s forward pass...')

    import os
    from time import time
    from opts import PreimageOpts
    from problem import Problem

    sha256_d8_sym = os.path.join('samples', 'sha256_d8_sym.txt')
    sha256_d8_cnf = os.path.join('samples', 'sha256_d8_cnf.txt')
    print('Loading problem: (%s, %s)' % (sha256_d8_sym, sha256_d8_cnf))

    opts = PreimageOpts()
    problem = Problem.from_files(sha256_d8_sym, sha256_d8_cnf)
    n_vars = problem.num_vars
    n_gates = problem.num_gates
    print(f'n_vars={n_vars}\nn_gates=m={n_gates}\nd={opts.d}')

    observed = problem.random_observed()
    model = NeuroSAT(opts, problem)

    cum_time = 0.0
    N = 1
    for _ in range(N):
        start = time()
        model(observed)
        cum_time += (time() - start)

    runtime = cum_time / float(N)
    print('Done! Average forward pass in %.2f ms' % (1000.0 * runtime))
