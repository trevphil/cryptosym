import torch

from mlp import MLP

class HashSAT(torch.nn.Module):
  def __init__(self, opts):
    super().__init__()
    self.opts = opts
    # LSTM
    self.L_lstm = torch.nn.LSTMCell(input_size=int(2 * opts.d),
                                    hidden_size=opts.d,
                                    bias=True)
    self.C_lstm = torch.nn.LSTMCell(input_size=opts.d,
                                    hidden_size=opts.d,
                                    bias=True)
    """
    self.M_lstm = torch.nn.LSTMCell(input_size=opts.d_master,
                                    hidden_size=opts.d_master,
                                    bias=True)
    """
    # Multi-layer perceptrons
    dims_out_msg = [opts.d for _ in range(opts.n_msg_layers)] + [opts.d]
    dims_out_vote = [opts.d for _ in range(opts.n_vote_layers)] + [1]
    self.L_msg = MLP(dim_in=opts.d, dims_out=dims_out_msg)
    self.C_msg = MLP(dim_in=opts.d, dims_out=dims_out_msg)
    # self.M_msg = MLP(dim_in=opts.d, dims_out=[opts.d_master])
    self.L_vote = MLP(dim_in=opts.d, dims_out=dims_out_vote)
    self.norm = 1.0 / (opts.d ** 0.5)
    # self.norm_master = 1.0 / (opts.d_master ** 0.5)
    # Sigmoid
    self.sigmoid = torch.nn.Sigmoid()
    
    num_param = sum(p.numel() for p in self.parameters())
    print(f'HashSAT has {num_param} parameters.')
  
  def flip(self, A):
    # Flip first and last half of rows in A
    half_rows = A.size(0) // 2
    return torch.cat((A[half_rows:], A[:half_rows]), axis=0)

  def forward(self, A, A_T, observed):
    dtype = self.opts.dtype
    n_lits, n_gates = A.size()[:2]  # n, m
    n_vars = n_lits // 2

    # Message tensors
    L_init = torch.normal(0, 1, size=(1, self.opts.d), dtype=dtype) * self.norm
    C_init = torch.normal(0, 1, size=(1, self.opts.d), dtype=dtype) * self.norm
    L_state = torch.tile(L_init, (n_lits, 1))   # n, d
    C_state = torch.tile(C_init, (n_gates, 1))  # m, d
    # M_state = torch.normal(0, 1, size=(1, self.opts.d_master), dtype=dtype) * self.norm_master
    
    for var, val in observed.items():
      val = float(val)
      L_state[var - 1, :] = val                 # Assign value of non-negated literal
      L_state[var - 1 + n_vars, :] = 1.0 - val  # Assign value of negated literal

    # Hidden states
    L_hidden = torch.zeros(n_lits, self.opts.d, dtype=dtype)   # n, d
    C_hidden = torch.zeros(n_gates, self.opts.d, dtype=dtype)  # m, d
    # M_hidden = torch.zeros(1, self.opts.d_master, dtype=dtype) # 1, d_master

    for _ in range(self.opts.T):
      L_msg_pre = self.L_msg(L_state).squeeze()  # n, d
      L_msg = torch.matmul(A_T, L_msg_pre)  # m, d

      C_hidden, C_state = self.C_lstm(L_msg, (C_hidden, C_state))

      C_msg_pre = self.C_msg(C_state).squeeze()  # m, d
      C_msg = torch.matmul(A, C_msg_pre)  # n, d
      C_msg = torch.cat((C_msg, self.flip(L_state)), axis=1)  # n, 2d
      
      L_hidden, L_state = self.L_lstm(C_msg, (L_hidden, L_state))
      
      """
      M_msg_pre = torch.cat((L_state, C_state), axis=0)  # m+n, d
      M_msg = self.M_msg(M_msg_pre)  # m+n, d_master
      M_msg = torch.mean(M_msg, dim=0).view(1, -1)
      
      M_hidden, M_state = self.M_lstm(M_msg, (M_hidden, M_state))
      """

    votes = self.L_vote(L_state).squeeze()  # n
    positive = votes[:n_vars]  # n/2
    negative = votes[n_vars:]  # n/2
    assignments = self.sigmoid(positive - negative)
    return assignments

if __name__ == '__main__':
  print('Testing the model\'s forward pass...')

  import os
  from time import time
  from config import HashSATConfig
  from problem import Problem
  
  opts = HashSATConfig()
  model = HashSAT(opts)
  
  sha256_d8_sym = os.path.join('samples', 'sha256_d8_sym.txt')
  sha256_d8_cnf = os.path.join('samples', 'sha256_d8_cnf.txt')
  print('Loading problem: (%s, %s)' % (sha256_d8_sym, sha256_d8_cnf))

  problem = Problem.from_files(sha256_d8_sym, sha256_d8_cnf)
  n_vars = problem.num_vars
  n_lits = int(2 * n_vars)
  n_gates = problem.num_gates
  print(f'n_vars={n_vars}\nn_lits=n={n_lits}\nn_gates=m={n_gates}\nd={opts.d}')
  
  problem.set_observed({1: True})

  cum_time = 0.0
  N = 1
  for _ in range(N):
    start = time()
    model(problem.A, problem.A_T, problem.observed)
    cum_time += (time() - start)

  runtime = cum_time / float(N)
  print('Done! Average forward pass in %.2f ms' % (1000.0 * runtime))
