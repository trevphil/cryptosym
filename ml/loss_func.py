import torch

def _get_val(y_pred, var):
  idx = abs(var) - 1
  val = y_pred[idx]
  if var < 0:
    val = 1.0 - val
  return val

class HashSATLoss(object):
  def __init__(self):
    pass
    
  def bce(self, y_pred, problem):
    loss = 0.0
    n = 0

    # Loss for constraints which are not satisfied
    for and_gate in problem.and_gates:
      inp1 = _get_val(y_pred, and_gate.inp1)
      inp2 = _get_val(y_pred, and_gate.inp2)
      out = _get_val(y_pred, and_gate.out)

      bce = -torch.log(1.0 - torch.abs(inp1 * inp2 - out))
      loss += torch.clamp(bce, min=-100.0)
      n += 1

    # Loss for observed variables
    for var, val in problem.observed.items():
      pred = _get_val(y_pred, var)
      bce = -torch.log(1.0 - torch.abs(int(val) - pred))
      loss += torch.clamp(bce, min=-100.0)
      n += 1

    return loss / n

  def mse(self, y_pred, problem):
    constraint_loss = 0.0
    observed_loss = 0.0
    
    # Loss for constraints which are not satisfied
    for and_gate in problem.and_gates:
      inp1 = _get_val(y_pred, and_gate.inp1)
      inp2 = _get_val(y_pred, and_gate.inp2)
      out = _get_val(y_pred, and_gate.out)

      gate_loss = (inp1 * inp2 - out) ** 2
      constraint_loss += gate_loss

    # Loss for observed variables
    for var, target in problem.observed.items():
      pred = _get_val(y_pred, var)
      observed_loss += (int(target) - pred) ** 2

    return constraint_loss + observed_loss

if __name__ == '__main__':
  print('Testing the loss function...')

  import os
  from time import time
  from config import HashSATConfig
  from problem import Problem
  from hashsat import HashSAT
  
  opts = HashSATConfig()
  model = HashSAT(opts)
  loss_fn = HashSATLoss()
  
  md5_d1_sym = os.path.join('samples', 'md5_d1_sym.txt')
  md5_d1_cnf = os.path.join('samples', 'md5_d1_cnf.txt')
  print('Loading problem: (%s, %s)' % (md5_d1_sym, md5_d1_cnf))

  problem = Problem.from_files(md5_d1_sym, md5_d1_cnf)
  n_vars = problem.num_vars
  n_lits = int(2 * n_vars)
  n_gates = problem.num_gates
  print(f'n_vars={n_vars}\nn_lits=n={n_lits}\nn_gates=m={n_gates}\nd={opts.d}')
  
  problem.set_observed({1: True, 2: False})
  y_pred = model(problem.A, problem.A_T, problem.observed)

  start = time()
  loss_bce = loss_fn.bce(y_pred, problem)
  runtime_bce = (time() - start) * 1000.0
  
  start = time()
  loss_mse = loss_fn.mse(y_pred, problem)
  runtime_mse = (time() - start) * 1000.0

  print('Done! loss_bce=%f (%.2f ms); loss_mse=%f (%.2f ms)' % (
      loss_bce, runtime_bce, loss_mse, runtime_mse))
