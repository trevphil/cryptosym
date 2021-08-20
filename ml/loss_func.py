import torch
from collections import defaultdict

from logic_gate import GateType, gate_type_to_str


def _get_val(pred_bits, literal):
    idx = abs(literal) - 1
    bit = pred_bits[idx]
    if literal < 0:
        bit = 1 - bit
    return bit


class PreimageLoss(object):
    def __init__(self, problem):
        self.problem = problem

    def bce(self, pred_bits, observed):
        cum_loss_per_gate_type = defaultdict(lambda: 0.0)
        count_per_gate_type = defaultdict(lambda: 0.0)

        # Loss for constraints which are not satisfied
        for gate in self.problem.gates:
            inputs = [_get_val(pred_bits, i) for i in gate.inputs]
            target = gate.compute_output(inputs)
            y = _get_val(pred_bits, gate.output)

            bce = -torch.log(1.0 - torch.abs(target - y))
            bce = torch.clamp(bce, min=-100.0)

            gate_type = gate_type_to_str(gate.t)
            cum_loss_per_gate_type[gate_type] += bce
            count_per_gate_type[gate_type] += 1

        # Loss for observed variables
        for var, target in observed.items():
            y = _get_val(pred_bits, var)
            bce = -torch.log(1.0 - torch.abs(int(target) - y))
            bce = torch.clamp(bce, min=-100.0)
            cum_loss_per_gate_type['observed'] += bce
            count_per_gate_type['observed'] += 1

        mean_loss_per_gate_type = dict()
        for gate_type, cum_loss in cum_loss_per_gate_type.items():
            count = count_per_gate_type[gate_type]
            mean_loss_per_gate_type[gate_type] = cum_loss / count

        total_loss = sum(cum_loss_per_gate_type.values())
        total_count = sum(count_per_gate_type.values())

        return total_loss / total_count, mean_loss_per_gate_type

    def sse(self, pred_bits, observed):
        cum_loss_per_gate_type = defaultdict(lambda: 0.0)

        # Loss for constraints which are not satisfied
        for gate in self.problem.gates:
            inputs = [_get_val(pred_bits, i) for i in gate.inputs]
            target = gate.compute_output(inputs)
            y = _get_val(pred_bits, gate.output)

            gate_loss = (target - y) ** 2
            gate_type = gate_type_to_str(gate.t)
            cum_loss_per_gate_type[gate_type] += gate_loss

        # Loss for observed variables
        for var, target in observed.items():
            y = _get_val(pred_bits, var)
            observed_loss = (int(target) - y) ** 2
            cum_loss_per_gate_type['observed'] += observed_loss

        total_sse = sum(cum_loss_per_gate_type.values())
        return total_sse, cum_loss_per_gate_type


if __name__ == '__main__':
    print('Testing the loss function...')

    import os
    from time import time
    from opts import PreimageOpts
    from problem import Problem
    from models.neurosat import NeuroSAT

    opts = PreimageOpts()

    sym = os.path.join('samples', 'sha256_d8_sym.txt')
    cnf = os.path.join('samples', 'sha256_d8_cnf.txt')
    print('Loading problem: (%s, %s)' % (sym, cnf))

    problem = Problem.from_files(sym, cnf)
    n_vars = problem.num_vars
    n_lits = int(2 * n_vars)
    n_gates = problem.num_gates
    print(f'n_vars={n_vars}\nn_lits=n={n_lits}\nn_gates=m={n_gates}\nd={opts.d}')

    model = NeuroSAT(opts, problem)
    observed = problem.random_observed()
    y_pred = model(observed)
    loss_fn = PreimageLoss(problem)

    start = time()
    loss_bce, _ = loss_fn.bce(y_pred, observed)
    runtime_bce = (time() - start) * 1000.0

    start = time()
    loss_sse, _ = loss_fn.sse(y_pred, observed)
    runtime_sse = (time() - start) * 1000.0

    print('Done! loss_bce=%f (%.2f ms); loss_sse=%f (%.2f ms)' % (
        loss_bce, runtime_bce, loss_sse, runtime_sse))
